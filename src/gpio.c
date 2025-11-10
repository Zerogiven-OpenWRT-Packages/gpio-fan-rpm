/**
 * @file gpio.c
 * @brief GPIO operations for fan RPM measurement
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 * 
 * This module implements GPIO operations with compatibility for both
 * libgpiod v1 (OpenWRT 23.05) and libgpiod v2 (OpenWRT 24.10).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <poll.h>
#include "gpio.h"
#include "chip.h"
#include "line.h"
#include "format.h"

// Global variables (extern declaration - defined in main.c)
extern volatile sig_atomic_t stop;
extern pthread_mutex_t print_mutex;

gpio_context_t* gpio_init(int gpio, const char *chipname) {
    gpio_context_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;
    
    ctx->gpio = gpio;
    
    if (chipname) {
        // Use specified chip
        ctx->chip = chip_open_by_name(chipname);
        if (!ctx->chip) {
            fprintf(stderr, "Error: cannot open chip '%s'\n", chipname);
            free(ctx);
            return NULL;
        }
        ctx->chipname = strdup(chipname);
        if (!ctx->chipname) {
            fprintf(stderr, "Error: memory allocation failed\n");
            chip_close(ctx->chip);
            free(ctx);
            return NULL;
        }
    } else {
        // Auto-detect chip (this should rarely happen now)
        ctx->chip = chip_auto_detect(gpio, &ctx->chipname);
        if (!ctx->chip) {
            fprintf(stderr, "Error: cannot find suitable chip for GPIO %d\n", gpio);
            free(ctx);
            return NULL;
        }
    }
    
    return ctx;
}

void gpio_cleanup(gpio_context_t *ctx) {
    if (!ctx) return;
    
#ifdef LIBGPIOD_V2
    if (ctx->request) {
        gpiod_line_request_release(ctx->request);
        ctx->request = NULL;
    }
    ctx->event_fd = -1;
#else
    if (ctx->line) {
        gpiod_line_release(ctx->line);
        ctx->line = NULL;
    }
#endif
    
    if (ctx->chip) {
        chip_close(ctx->chip);
        ctx->chip = NULL;
    }
    
    if (ctx->chipname) {
        free(ctx->chipname);
        ctx->chipname = NULL;
    }
    
    free(ctx);
}

int gpio_request_events(gpio_context_t *ctx, const char *consumer) {
    if (!ctx || !ctx->chip) return -1;
    
    // Use the line module to request events
    line_request_t *line_req = line_request_events(ctx->chip, ctx->gpio, consumer);
    if (!line_req) return -1;
    
#ifdef LIBGPIOD_V2
    ctx->request = line_req->request;
    ctx->event_fd = line_req->event_fd;
#else
    ctx->line = line_req->line;
#endif
    
    // Free the line request wrapper (but keep the underlying request)
    free(line_req);
    
    return 0;
}

int gpio_wait_event(gpio_context_t *ctx, int64_t timeout_ns) {
    if (!ctx) return -1;
    
#ifdef LIBGPIOD_V2
    if (ctx->event_fd < 0) return -1;
    
    struct pollfd pfd;
    pfd.fd = ctx->event_fd;
    pfd.events = POLLIN;
    
    int timeout_ms = (timeout_ns >= 0) ? (timeout_ns / 1000000LL) : -1;
    int ret = poll(&pfd, 1, timeout_ms);
    
    if (ret < 0) return -1;  // Error
    if (ret == 0) return 0;  // Timeout
    return 1;  // Event available
#else
    if (!ctx->line) return -1;
    
    struct timespec ts;
    ts.tv_sec = timeout_ns / 1000000000LL;
    ts.tv_nsec = timeout_ns % 1000000000LL;
    
    return gpiod_line_event_wait(ctx->line, &ts);
#endif
}

int gpio_read_event(gpio_context_t *ctx) {
    if (!ctx) return -1;
    
#ifdef LIBGPIOD_V2
    if (!ctx->request) return -1;
    
    // For libgpiod v2, we need to use an event buffer
    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
    if (!buffer) return -1;
    
    int ret = gpiod_line_request_read_edge_events(ctx->request, buffer, 1);
    if (ret > 0) {
        // Get the event from the buffer
        struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(buffer, 0);
        if (event) {
            // Process event if needed
            // For now, we just count it
        }
    }
    
    gpiod_edge_event_buffer_free(buffer);
    return ret;
#else
    if (!ctx->line) return -1;
    
    struct gpiod_line_event event;
    return gpiod_line_event_read(ctx->line, &event);
#endif
}

/**
 * @brief Measure RPM using GPIO edge detection
 * 
 * This function performs a two-phase measurement:
 * 1. Warmup phase: 1 second for fan stabilization
 * 2. Measurement phase: (duration-1) seconds for actual RPM calculation
 * 
 * @param ctx GPIO context
 * @param pulses_per_rev Pulses per revolution
 * @param duration Total measurement duration in seconds (minimum 2)
 * @param debug Enable debug output
 * @return double RPM value, or -1.0 if interrupted
 */
double gpio_measure_rpm(gpio_context_t *ctx, int pulses_per_rev, int duration, int debug) {
    if (!ctx) return 0.0;
    
    struct timespec start_ts;
    unsigned int count = 0;
    int warmup_duration = 1;  // Fixed 1-second warmup
    int measurement_duration = duration - warmup_duration;
    int measurement_completed = 0;
    
    // Warmup phase
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    if (debug) {
        fprintf(stderr, "Warmup phase: %d seconds\n", warmup_duration);
    }
    
    // Warmup phase - simple 1-second wait
    struct timespec warmup_end_ts = start_ts;
    warmup_end_ts.tv_sec += warmup_duration;

    while (!stop) {
        struct timespec current_ts;
        clock_gettime(CLOCK_MONOTONIC, &current_ts);
        if (current_ts.tv_sec >= warmup_end_ts.tv_sec) {
            break; // Warmup time completed
        }

        // Wait for events during warmup (non-blocking)
        int wait_result = gpio_wait_event(ctx, 100000000LL); // 100ms timeout
        if (wait_result > 0) {
            gpio_read_event(ctx); // Only read if event is available
        }
        // If timeout (wait_result == 0) or error (wait_result < 0), just continue
    }

    // Check if interrupted during warmup
    if (stop) {
        return -1.0; // Interrupted during warmup
    }
    
    // Measurement phase - run for full measurement duration
    count = 0;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    if (debug) {
        fprintf(stderr, "Measurement phase: %d seconds\n", measurement_duration);
    }
    
    struct timespec measurement_end_ts;
    measurement_end_ts = start_ts;
    measurement_end_ts.tv_sec += measurement_duration;
    
    while (!stop) {
        // Check if measurement time is up
        struct timespec current_ts;
        clock_gettime(CLOCK_MONOTONIC, &current_ts);
        if (current_ts.tv_sec >= measurement_end_ts.tv_sec) {
            measurement_completed = 1;
            break; // Measurement time completed
        }
        
        int wait_result = gpio_wait_event(ctx, 1000000000LL);
        if (wait_result <= 0) {
            // Timeout - continue until measurement time is up
            continue;
        }
        if (gpio_read_event(ctx) < 0) {
            if (debug) fprintf(stderr, "Warning: error reading event during measurement\n");
            break;
        }
        
        count++;
    }
    
    // If we were interrupted before completing the measurement, return special value
    if (!measurement_completed && stop) {
        return -1.0; // Special value to indicate interruption
    }
    
    // Get current time for elapsed calculation
    struct timespec current_ts;
    clock_gettime(CLOCK_MONOTONIC, &current_ts);
    
    double elapsed = (current_ts.tv_sec - start_ts.tv_sec) + 
                     (current_ts.tv_nsec - start_ts.tv_nsec) / 1e9;
    
    if (elapsed <= 0.0) return 0.0;
    
    // Calculate RPM: (pulses / pulses_per_rev) / time * 60
    // This is equivalent to: frequency(Hz) * 60 / pulses_per_rev
    double revs = (double)count / pulses_per_rev;
    double rpm = revs / elapsed * 60.0;
    
    if (debug) {
        fprintf(stderr, "Counted %u pulses in %.3f s, RPM=%.1f\n", 
                count, elapsed, rpm);
        fprintf(stderr, "  Pulses per revolution: %d\n", pulses_per_rev);
        fprintf(stderr, "  Revolutions: %.2f\n", revs);
        fprintf(stderr, "  Frequency: %.2f Hz\n", count / elapsed);
    }
    
    return rpm;
}

void* gpio_thread_fn(void *arg) {
    thread_args_t *a = arg;
    gpio_context_t *ctx;
    
    // Initialize GPIO context
    ctx = gpio_init(a->gpio, a->chipname);
    if (!ctx) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Error: cannot open chip for GPIO %d\n", a->gpio);
        pthread_mutex_unlock(&print_mutex);
        free(a);
        return NULL;
    }
    
    // Request edge events
    if (gpio_request_events(ctx, "gpio-fan-rpm") < 0) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Error: cannot request events for GPIO %d\n", a->gpio);
        pthread_mutex_unlock(&print_mutex);
        gpio_cleanup(ctx);
        free(a);
        return NULL;
    }
    
    // Warmup once for watch mode
    if (a->watch) {
        gpio_measure_rpm(ctx, a->pulses, a->duration, a->debug);
    }
    
    // Measurement loop
    do {
        double rpm = gpio_measure_rpm(ctx, a->pulses, a->duration, a->debug);
        

        
        // Don't output interrupted measurements
        if (rpm < 0.0) {
            // Interrupted during measurement, exit cleanly
            break;
        }
        
        // For multiple GPIOs, store result and signal completion
        if (a->total_threads > 1 && a->results && a->finished && 
            a->results_mutex && a->all_finished) {
            pthread_mutex_lock(a->results_mutex);
            
            // Store result
            a->results[a->thread_index] = rpm;
            a->finished[a->thread_index] = 1;
            
            // Check if all threads finished
            int all_done = 1;
            for (size_t i = 0; i < a->total_threads; i++) {
                if (!a->finished[i]) {
                    all_done = 0;
                    break;
                }
            }
            
            if (all_done) {
                int ret = pthread_cond_signal(a->all_finished);
                if (ret != 0) {
                    fprintf(stderr, "Warning: pthread_cond_signal failed: %s\n", strerror(ret));
                }
            }

            pthread_mutex_unlock(a->results_mutex);
            

        } else {
            // Single GPIO - store result for main thread to output
            // This prevents double output in single measurement mode
            if (a->results && a->finished) {
                pthread_mutex_lock(a->results_mutex);
                a->results[a->thread_index] = rpm;
                a->finished[a->thread_index] = 1;
                pthread_mutex_unlock(a->results_mutex);
            } else {
                // Fallback: direct output if no synchronization primitives available
                pthread_mutex_lock(&print_mutex);
                char *output = format_output(a->gpio, rpm, a->mode, a->duration);
                if (output) {
                    printf("%s", output);
                    free(output);
                }
                fflush(stdout);
                pthread_mutex_unlock(&print_mutex);
            }
        }
        
        // For single measurement mode, only run once
        if (!a->watch) {
            break;
        }
        
    } while (a->watch && !stop);
    
    gpio_cleanup(ctx);
    free(a);
    return NULL;
} 