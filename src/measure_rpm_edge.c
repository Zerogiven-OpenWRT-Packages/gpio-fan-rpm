// measure_rpm_edge.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <pthread.h>

// Include our compatibility header first
#include "gpiod_compat.h"
#include <gpiod.h> // Include system gpiod.h first to get correct definitions

#include "gpio-fan-rpm.h"

// Define constants if not already defined in the system headers
#ifndef GPIOD_LINE_EVENT_RISING_EDGE
#define GPIOD_LINE_EVENT_RISING_EDGE 1
#endif

#ifndef GPIOD_LINE_EVENT_FALLING_EDGE
#define GPIOD_LINE_EVENT_FALLING_EDGE 2
#endif

// Function to measure RPM using edge detection method (common for both v1/v2)
static void *edge_measure_thread(void *arg) {
    edge_thread_args_t *args = (edge_thread_args_t *)arg;
    gpio_info_t *info = args->info;
    
    char chip_path[128];
    snprintf(chip_path, sizeof(chip_path), "/dev/%s", info->chip);
    
    if (args->debug)
        fprintf(stderr, "[DEBUG] GPIO %d: Opening chip %s\n", info->gpio_rel, chip_path);
    
    struct gpiod_chip *chip = gpiod_chip_open(chip_path);
    if (!chip) {
        fprintf(stderr, "[ERROR] Failed to open chip: %s\n", chip_path);
        args->success = 0;
        return NULL;
    }
    
    struct gpiod_line *line = NULL;
    int fd = -1;
    int ret = -1;
    
    // Get line handle
    line = compat_gpiod_chip_get_line(chip, info->gpio_rel);
    if (!line) {
        fprintf(stderr, "[ERROR] Failed to get line %d on %s\n", info->gpio_rel, chip_path);
        args->success = 0;
        goto cleanup;
    }
    
    // Request line for monitoring both edges or fallback to falling only if both fails
    if (args->debug)
        fprintf(stderr, "[DEBUG] Requesting line for edge monitoring\n");
    
    ret = compat_gpiod_line_request_both_edges_events(line, "gpio-fan-rpm");
    if (ret < 0) {
        if (args->debug)
            fprintf(stderr, "[DEBUG] Both edges failed, trying falling edge only\n");
        
        ret = compat_gpiod_line_request_falling_edge_events(line, "gpio-fan-rpm");
        if (ret < 0) {
            fprintf(stderr, "[ERROR] Failed to request line for edge events\n");
            args->success = 0;
            goto cleanup;
        }
    }
    
    // Get file descriptor for polling
    fd = compat_gpiod_line_event_get_fd(line);
    if (fd < 0) {
        fprintf(stderr, "[ERROR] Could not get event fd for line %d on %s\n", info->gpio_rel, chip_path);
        compat_gpiod_line_release(line); // Release line if request succeeded but fd failed
        args->success = 0;
        goto cleanup;
    }
    
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int count = 0;
    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    
    if (args->debug) {
        fprintf(stderr, "[DEBUG] GPIO %d: Listening for edges (%d sec, %d pulses/rev)\n",
                info->gpio_rel, args->duration, args->pulses_per_rev);
    }
    
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_sec = now.tv_sec - start.tv_sec;
        if (elapsed_sec >= args->duration)
            break;
        
        // Calculate remaining time for poll timeout more accurately
        long remaining_ms = (args->duration * 1000) - ((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000);
        if (remaining_ms <= 0) break; // Time's up exactly
        if (remaining_ms > 1000) remaining_ms = 1000; // Cap poll timeout
        
        int ret = poll(&pfd, 1, remaining_ms);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("[ERROR] poll()");
            break;
        }
        else if (ret == 0) {
            // Timeout - check if overall duration is met before continuing
            clock_gettime(CLOCK_MONOTONIC, &now);
            if ((now.tv_sec - start.tv_sec) >= args->duration) break;
            continue;
        }
        
        // Event occurred
        if (pfd.revents & POLLIN) {
            struct compat_gpiod_line_event event;
            
            ret = compat_gpiod_line_event_read(line, &event);
            if (ret < 0) {
                perror("[ERROR] Failed to read line event");
                break;
            }
            
            count++;
        }
    }
    
    args->rpm_out = (count > 0 && args->pulses_per_rev > 0 && args->duration > 0)
                   ? (count * 60) / args->pulses_per_rev / args->duration
                   : 0;
    
    if (args->debug) {
        fprintf(stderr, "[DEBUG] GPIO %d: Counted %d edges\n", info->gpio_rel, count);
        fprintf(stderr, "[DEBUG] GPIO %d: RPM = %d (count=%d, pulses/rev=%d, duration=%d)\n",
                info->gpio_rel, args->rpm_out, count, args->pulses_per_rev, args->duration);
    }
    
    args->success = 1;
    
cleanup:
    if (line && fd >= 0)
        compat_gpiod_line_release(line);
    if (chip)
        gpiod_chip_close(chip);
    return NULL;
}

// Main measure_rpm_edge function - used by the CLI and daemon
float measure_rpm_edge(const char *chip_name, int pin, int debug_level) {
    if (!chip_name || pin < 0) {
        fprintf(stderr, "[ERROR] Invalid chip name or pin\n");
        return -1.0f;
    }
    
    gpio_info_t info = {0};
    strncpy(info.chip, chip_name, sizeof(info.chip) - 1);
    info.gpio_rel = pin;
    
    // Use the common edge detection method
    edge_thread_args_t args = {
        .info = &info,
        .pulses_per_rev = 2,  // Default: 2 pulses per revolution
        .duration = 2,        // Default: 2 seconds measurement duration
        .debug = debug_level,
        .success = 0,
        .rpm_out = 0
    };
    
    // Run measurement in current thread (simpler than creating a new thread)
    edge_measure_thread(&args);
    
    if (!args.success) {
        fprintf(stderr, "[ERROR] Failed to measure RPM\n");
        return -1.0f;
    }
    
    return (float)args.rpm_out;
}
