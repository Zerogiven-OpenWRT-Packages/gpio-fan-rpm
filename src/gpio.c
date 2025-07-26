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
#include <pthread.h>
#include <poll.h>
#include "gpio.h"
#include "chip.h"
#include "line.h"
#include "format.h"

// Global variables
volatile int stop = 0;
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
    } else {
        // Auto-detect chip
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
    }
#else
    if (ctx->line) {
        gpiod_line_release(ctx->line);
    }
#endif
    
    if (ctx->chip) {
        chip_close(ctx->chip);
    }
    
    if (ctx->chipname) {
        free(ctx->chipname);
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

double gpio_measure_rpm(gpio_context_t *ctx, int pulses_per_rev, int duration, int debug) {
    if (!ctx) return 0.0;
    
    struct timespec start_ts, ev_ts;
    unsigned int count = 0;
    int half = duration / 2;
    
    // Warmup phase
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    while (1) {
        if (gpio_wait_event(ctx, 1000000000LL) <= 0) break; // 1 second timeout
        if (gpio_read_event(ctx) < 0) break;
        
        clock_gettime(CLOCK_MONOTONIC, &ev_ts);
        if ((ev_ts.tv_sec - start_ts.tv_sec) >= half) break;
    }
    
    // Measurement phase
    count = 0;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    while (!stop) {
        if (gpio_wait_event(ctx, 1000000000LL) <= 0) break; // 1 second timeout
        if (gpio_read_event(ctx) < 0) break;
        
        count++;
        clock_gettime(CLOCK_MONOTONIC, &ev_ts);
        if ((ev_ts.tv_sec - start_ts.tv_sec) >= half) break;
    }
    
    double elapsed = (ev_ts.tv_sec - start_ts.tv_sec) + 
                     (ev_ts.tv_nsec - start_ts.tv_nsec) / 1e9;
    
    if (elapsed <= 0.0) return 0.0;
    
    double revs = (double)count / pulses_per_rev;
    double rpm = revs / elapsed * 60.0;
    
    if (debug) {
        fprintf(stderr, "Counted %u pulses in %.3f s, RPM=%.1f\n", 
                count, elapsed, rpm);
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
                pthread_cond_signal(a->all_finished);
            }
            
            pthread_mutex_unlock(a->results_mutex);
            

        } else {
            // Single GPIO or no synchronization primitives - use original approach
            pthread_mutex_lock(&print_mutex);
            char *output = NULL;
            
            switch (a->mode) {
            case MODE_NUMERIC:
                output = format_numeric(rpm);
                break;
            case MODE_JSON:
                output = format_json(a->gpio, rpm);
                break;
            case MODE_COLLECTD:
                output = format_collectd(a->gpio, rpm, a->duration);
                break;
            default:
                output = format_human_readable(a->gpio, rpm);
                break;
            }
            
            if (output) {
                printf("%s", output);
                free(output);
            }
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
        }
        
    } while (a->watch && !stop);
    
    gpio_cleanup(ctx);
    free(a);
    return NULL;
} 