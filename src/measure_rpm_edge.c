// measure_rpm_edge.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <pthread.h>
#include <gpiod.h>

// Include our compatibility header
#include "gpio_compat.h"
#include "gpio-fan-rpm.h"

// Function to measure RPM using edge detection method
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
    
    // Get line handle using our compatibility function
    line = gpio_compat_get_line(chip, info->gpio_rel);
    if (!line) {
        fprintf(stderr, "[ERROR] Failed to get line %d on %s\n", info->gpio_rel, chip_path);
        args->success = 0;
        goto cleanup;
    }
    
    // Request line for monitoring both edges or fallback to falling only if both fails
    if (args->debug)
        fprintf(stderr, "[DEBUG] Requesting line for edge monitoring\n");
    
    // Use our compatibility function to request events
    ret = gpio_compat_request_events(line, "gpio-fan-rpm", 0); // Try both edges first
    if (ret < 0) {
        if (args->debug)
            fprintf(stderr, "[DEBUG] Both edges failed, trying falling edge only\n");
        
        ret = gpio_compat_request_events(line, "gpio-fan-rpm", 1); // Try falling edge only
        if (ret < 0) {
            fprintf(stderr, "[ERROR] Failed to request line for edge events\n");
            args->success = 0;
            goto cleanup;
        }
    }
    
    // Get file descriptor for polling using our compatibility function
    fd = gpio_compat_get_fd(line);
    if (fd < 0) {
        fprintf(stderr, "[ERROR] Could not get event fd for line %d on %s\n", info->gpio_rel, chip_path);
        gpio_compat_release_line(line); // Release line if request succeeded but fd failed
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
        
        // Calculate remaining time for poll timeout
        long remaining_ms = (args->duration * 1000) - ((now.tv_sec - start.tv_sec) * 1000 + 
                           (now.tv_nsec - start.tv_nsec) / 1000000);
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
            struct gpio_compat_event event;
            
            ret = gpio_compat_read_event(line, &event);
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
        gpio_compat_release_line(line);
    if (chip)
        gpiod_chip_close(chip);
    return NULL;
}

#else // --- Fallback to libgpiod v1 API ---

#if defined(DEBUG)
static const char v1_fallback_msg[] __attribute__((unused)) = 
    "Note: Compiling with libgpiod v1 API fallback. GPIO bias settings might not be applied.";
#endif

// Define missing v1 API prototypes and structures
struct gpiod_chip *gpiod_chip_open(const char *path);
void gpiod_chip_close(struct gpiod_chip *chip);
struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer);
int gpiod_line_event_get_fd(struct gpiod_line *line);
void gpiod_line_release(struct gpiod_line *line);

// Define the missing gpiod_line_event structure for v1 API
struct gpiod_line_event {
    struct timespec ts;
    int event_type;
};

int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event);

static void *edge_measure_thread_v1(void *arg) {
    edge_thread_args_t *args = (edge_thread_args_t *)arg;
    gpio_info_t *info = args->info;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line *line = NULL;
    int fd = -1;
    int ret;

    char chip_path[128];
    snprintf(chip_path, sizeof(chip_path), "/dev/%s", info->chip);

    if (args->debug)
        fprintf(stderr, "[DEBUG-v1] GPIO %d: Opening chip %s\n", info->gpio_rel, chip_path);

    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        fprintf(stderr, "[ERROR-v1] Failed to open chip %s: %s\n", chip_path, strerror(errno));
        args->success = 0;
        return NULL;
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
    
    // Run measurement in current thread
    edge_measure_thread(&args);
    
    if (!args.success) {
        fprintf(stderr, "[ERROR] Failed to measure RPM\n");
        return -1.0f;
    }
    
    return (float)args.rpm_out;
}
