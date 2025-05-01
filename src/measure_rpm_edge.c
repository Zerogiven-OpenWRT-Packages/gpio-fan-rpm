// measure_rpm_edge.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include "gpiod_compat.h"  // Include our compatibility layer first
#include <pthread.h>

#include "gpio-fan-rpm.h"

// Edge-based RPM measurement with support for both libgpiod APIs
static void *edge_measure_thread(void *arg)
{
    edge_thread_args_t *args = (edge_thread_args_t *)arg;
    gpio_info_t *info = args->info;
    struct gpiod_chip *chip = NULL;
    int fd = -1;
    int ret;

    char chip_path[128];
    snprintf(chip_path, sizeof(chip_path), "/dev/%s", info->chip);

    if (args->debug)
        fprintf(stderr, "[DEBUG] GPIO %d: Opening chip %s\n", info->gpio_rel, chip_path);

    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        fprintf(stderr, "[ERROR] Failed to open chip %s: %s\n", chip_path, strerror(errno));
        args->success = 0;
        return NULL;
    }

#ifdef USING_LIBGPIOD_V2
    // libgpiod v2 native approach
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    struct gpiod_line_request *request = NULL;

    if (!settings || !line_cfg || !req_cfg) {
        fprintf(stderr, "[ERROR] Failed to allocate GPIO settings\n");
        gpiod_chip_close(chip);
        args->success = 0;
        return NULL;
    }

    // Configure for edge detection
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    unsigned int offset = info->gpio_rel;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
    gpiod_request_config_set_consumer(req_cfg, "gpio-fan-rpm");

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    
    // Clean up configuration objects
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
    
    if (!request) {
        fprintf(stderr, "[ERROR] Failed to request GPIO line %d: %s\n", 
                info->gpio_rel, strerror(errno));
        gpiod_chip_close(chip);
        args->success = 0;
        return NULL;
    }

    fd = gpiod_line_request_get_fd(request);
    if (fd < 0) {
        fprintf(stderr, "[ERROR] Failed to get file descriptor: %s\n", strerror(errno));
        gpiod_line_request_release(request);
        gpiod_chip_close(chip);
        args->success = 0;
        return NULL;
    }
#else
    // libgpiod v1 approach
    struct gpiod_line *line = gpiod_chip_get_line(chip, info->gpio_rel);
    if (!line) {
        fprintf(stderr, "[ERROR] Failed to get GPIO line %d: %s\n", 
                info->gpio_rel, strerror(errno));
        gpiod_chip_close(chip);
        args->success = 0;
        return NULL;
    }

    // Try both edges first, then fall back to falling edge
    ret = gpiod_line_request_both_edges_events(line, "gpio-fan-rpm");
    if (ret < 0) {
        if (args->debug)
            fprintf(stderr, "[DEBUG] Both edges failed, trying falling edge\n");
            
        ret = gpiod_line_request_falling_edge_events(line, "gpio-fan-rpm");
        if (ret < 0) {
            fprintf(stderr, "[ERROR] Failed to request edge events: %s\n", strerror(errno));
            gpiod_chip_close(chip);
            args->success = 0;
            return NULL;
        }
    }

    fd = gpiod_line_event_get_fd(line);
    if (fd < 0) {
        fprintf(stderr, "[ERROR] Failed to get file descriptor: %s\n", strerror(errno));
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        args->success = 0;
        return NULL;
    }
#endif

    // Measurement setup
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int count = 0;

    if (args->debug) {
        fprintf(stderr, "[DEBUG] GPIO %d: Measuring edges for %d second(s)\n", 
                info->gpio_rel, args->duration);
    }

    // Main measurement loop
    while (1) {
        // Check if we've measured long enough
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_sec = now.tv_sec - start.tv_sec;
        if (elapsed_sec >= args->duration)
            break;

        // Calculate poll timeout
        long remain_ms = (args->duration - elapsed_sec) * 1000 -
                        (now.tv_nsec - start.tv_nsec) / 1000000;
        if (remain_ms <= 0) break;
        if (remain_ms > 1000) remain_ms = 1000;  // Cap poll timeout at 1 second

        // Poll for events
        struct pollfd pfd = {.fd = fd, .events = POLLIN};
        ret = poll(&pfd, 1, remain_ms);

        if (ret < 0) {
            if (errno == EINTR) continue;  // Interrupted syscall
            fprintf(stderr, "[ERROR] poll() failed: %s\n", strerror(errno));
            break;
        } else if (ret == 0) {
            continue;  // Timeout
        }

        // Event available
        if (pfd.revents & POLLIN) {
#ifdef USING_LIBGPIOD_V2
            // Read events with v2 API
            struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(5);
            if (!buffer) {
                fprintf(stderr, "[ERROR] Failed to create event buffer\n");
                break;
            }

            ret = gpiod_line_request_read_edge_events(request, buffer, 5);
            if (ret < 0) {
                fprintf(stderr, "[ERROR] Failed to read events: %s\n", strerror(errno));
                gpiod_edge_event_buffer_free(buffer);
                break;
            }
            count += ret;  // Add number of events read
            gpiod_edge_event_buffer_free(buffer);
#else
            // Read events with v1 API
            struct gpiod_line_event event;
            ret = gpiod_line_event_read(line, &event);
            if (ret == 0) {
                count++;  // Successfully read an event
            } else if (ret < 0 && errno != EAGAIN && errno != EINTR) {
                fprintf(stderr, "[ERROR] Failed to read event: %s\n", strerror(errno));
                break;
            }
#endif
        }
    }

    // Cleanup
#ifdef USING_LIBGPIOD_V2
    gpiod_line_request_release(request);
#else
    gpiod_line_release(line);
#endif
    gpiod_chip_close(chip);

    // Calculate RPM from pulses
    if (count > 0 && args->pulses_per_rev > 0 && args->duration > 0) {
        args->rpm_out = (count * 60) / args->pulses_per_rev / args->duration;
    } else {
        args->rpm_out = 0;
    }

    if (args->debug) {
        fprintf(stderr, "[DEBUG] GPIO %d: Counted %d edges\n", info->gpio_rel, count);
        fprintf(stderr, "[DEBUG] GPIO %d: Calculated %d RPM\n", info->gpio_rel, args->rpm_out);
    }

    args->success = 1;
    return NULL;
}

// Public function to measure RPM on all GPIOs in parallel
int measure_rpm_edge(gpio_info_t *infos, int count, int duration, int debug)
{
    pthread_t threads[MAX_GPIOS];
    edge_thread_args_t args[MAX_GPIOS];

    if (count > MAX_GPIOS) {
        fprintf(stderr, "[ERROR] Too many GPIOs specified (max %d)\n", MAX_GPIOS);
        return -1;
    }

    if (debug) {
#ifdef USING_LIBGPIOD_V2
        fprintf(stderr, "[DEBUG] Using libgpiod v2 API\n");
#else
        fprintf(stderr, "[DEBUG] Using libgpiod v1 API\n");
#endif
    }

    // Create a thread for each GPIO
    int threads_created = 0;
    for (int i = 0; i < count; i++) {
        args[i].info = &infos[i];
        args[i].duration = duration;
        args[i].pulses_per_rev = infos[i].pulses_per_rev;
        args[i].debug = debug;
        args[i].rpm_out = 0;
        args[i].success = 0;

        if (debug) {
            fprintf(stderr, "[DEBUG] Starting thread for GPIO %d (chip=%s)\n", 
                    infos[i].gpio_rel, infos[i].chip);
        }

        if (pthread_create(&threads[i], NULL, edge_measure_thread, &args[i]) != 0) {
            fprintf(stderr, "[ERROR] Failed to create thread for GPIO %d\n", infos[i].gpio_rel);
            count = i;  // Only join threads created so far
            goto cleanup;
        }
        threads_created++;
    }

cleanup:
    // Wait for threads to complete
    for (int i = 0; i < threads_created; i++) {
        pthread_join(threads[i], NULL);
        
        if (args[i].success) {
            infos[i].rpm = args[i].rpm_out;
            infos[i].valid = 1;
        } else {
            infos[i].rpm = 0;
            infos[i].valid = 0;
        }
    }

    return (threads_created == count) ? 0 : -1;
}
