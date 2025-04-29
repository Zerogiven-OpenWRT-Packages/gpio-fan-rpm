// measure_rpm_edge.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <gpiod.h> // Include system gpiod.h first to get correct definitions
#include <pthread.h>

#include "gpio-fan-rpm.h"

// Structure to pass arguments to edge detection thread
typedef struct {
    gpio_info_t *info;
    int pulses_per_rev;
    int duration;
    int debug;
    int success;
    int rpm_out;
} edge_thread_args_t;

// Define constants if not already defined in the system headers
#ifndef GPIOD_LINE_EVENT_RISING_EDGE
#define GPIOD_LINE_EVENT_RISING_EDGE 1
#endif

#ifndef GPIOD_LINE_EVENT_FALLING_EDGE
#define GPIOD_LINE_EVENT_FALLING_EDGE 2
#endif

// Declare v1 API functions for v2 environment
#ifdef GPIOD_API_VERSION
    // We're in a v2 environment (OpenWRT 24.10)
    
    // We need to provide a v1-compatible event structure
    #ifndef HAVE_GPIOD_LINE_EVENT_STRUCT
    struct gpiod_line_event {
        struct timespec ts;
        int event_type;
    };
    #define HAVE_GPIOD_LINE_EVENT_STRUCT 1
    #endif
    
    // V1 API function prototypes - define these for v2 compatibility
    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
    int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
    int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer);
    int gpiod_line_event_get_fd(struct gpiod_line *line);
    void gpiod_line_release(struct gpiod_line *line);
    int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event);
    
    // Static storage for v2 compatibility
    static struct gpiod_line_settings *v1_compat_settings = NULL;
    static struct gpiod_line_config *v1_compat_line_cfg = NULL;
    static struct gpiod_request_config *v1_compat_req_cfg = NULL;
    static struct gpiod_line_request *v1_compat_request = NULL;
    
    // Implementation of v1 compatibility functions for v2 API
    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset) {
        // In v2, we don't get individual lines but use offsets during requests
        // Store info for later use in a static variable
        static struct {
            struct gpiod_chip *chip;
            unsigned int offset;
            int valid;
        } line_info = {0};
        
        if (chip) {
            line_info.chip = chip;
            line_info.offset = offset;
            line_info.valid = 1;
            return (struct gpiod_line *)&line_info;
        }
        return NULL;
    }
    
    int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer) {
        // In v2, requests are handled differently - we defer to event_get_fd
        (void)line;
        (void)consumer;
        return 0; // Pretend it worked
    }
    
    int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer) {
        // In v2, requests are handled differently - we defer to event_get_fd
        (void)line;
        (void)consumer;
        return 0; // Pretend it worked
    }
    
    int gpiod_line_event_get_fd(struct gpiod_line *line) {
        if (!line) return -1;
        
        // Extract the line_info we stored in gpiod_chip_get_line
        struct {
            struct gpiod_chip *chip;
            unsigned int offset;
            int valid;
        } *line_info = (void *)line;
        
        if (!line_info->valid || !line_info->chip) return -1;
        
        // Clean up any existing request
        if (v1_compat_request) {
            gpiod_line_request_release(v1_compat_request);
            v1_compat_request = NULL;
        }
        
        // This is where we actually make the request in v2 style
        v1_compat_settings = gpiod_line_settings_new();
        if (!v1_compat_settings) return -1;
        
        v1_compat_line_cfg = gpiod_line_config_new();
        if (!v1_compat_line_cfg) {
            gpiod_line_settings_free(v1_compat_settings);
            v1_compat_settings = NULL;
            return -1;
        }
        
        v1_compat_req_cfg = gpiod_request_config_new();
        if (!v1_compat_req_cfg) {
            gpiod_line_settings_free(v1_compat_settings);
            gpiod_line_config_free(v1_compat_line_cfg);
            v1_compat_settings = NULL;
            v1_compat_line_cfg = NULL;
            return -1;
        }
        
        gpiod_line_settings_set_direction(v1_compat_settings, GPIOD_LINE_DIRECTION_INPUT);
        gpiod_line_settings_set_edge_detection(v1_compat_settings, GPIOD_LINE_EDGE_BOTH);
        gpiod_line_settings_set_bias(v1_compat_settings, GPIOD_LINE_BIAS_PULL_UP);
        
        unsigned int offset = line_info->offset;
        gpiod_line_config_add_line_settings(v1_compat_line_cfg, &offset, 1, v1_compat_settings);
        gpiod_request_config_set_consumer(v1_compat_req_cfg, "gpio-fan-rpm");
        
        v1_compat_request = gpiod_chip_request_lines(line_info->chip, v1_compat_req_cfg, v1_compat_line_cfg);
        if (!v1_compat_request) {
            gpiod_line_settings_free(v1_compat_settings);
            gpiod_line_config_free(v1_compat_line_cfg);
            gpiod_request_config_free(v1_compat_req_cfg);
            v1_compat_settings = NULL;
            v1_compat_line_cfg = NULL;
            v1_compat_req_cfg = NULL;
            return -1;
        }
        
        return gpiod_line_request_get_fd(v1_compat_request);
    }
    
    void gpiod_line_release(struct gpiod_line *line) {
        (void)line; // Unused
        
        // Clean up v2 resources
        if (v1_compat_request) {
            gpiod_line_request_release(v1_compat_request);
            v1_compat_request = NULL;
        }
        if (v1_compat_settings) {
            gpiod_line_settings_free(v1_compat_settings);
            v1_compat_settings = NULL;
        }
        if (v1_compat_line_cfg) {
            gpiod_line_config_free(v1_compat_line_cfg);
            v1_compat_line_cfg = NULL;
        }
        if (v1_compat_req_cfg) {
            gpiod_request_config_free(v1_compat_req_cfg);
            v1_compat_req_cfg = NULL;
        }
    }
    
    int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event) {
        (void)line; // Unused
        
        if (!event || !v1_compat_request) return -1;
        
        // Read edge events using v2 API
        struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
        if (!buffer) return -1;
        
        int ret = gpiod_line_request_read_edge_events(v1_compat_request, buffer, 1);
        if (ret <= 0) {
            gpiod_edge_event_buffer_free(buffer);
            return (ret < 0) ? ret : -1;
        }
        
        // Convert to v1 event format
        const struct gpiod_edge_event *v2_event = gpiod_edge_event_buffer_get_event(buffer, 0);
        if (v2_event) {
            event->ts = gpiod_edge_event_get_timestamp(v2_event);
            
            int event_type = gpiod_edge_event_get_event_type(v2_event);
            if (event_type == GPIOD_EDGE_EVENT_RISING_EDGE) {
                event->event_type = GPIOD_LINE_EVENT_RISING_EDGE;
            } else {
                event->event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
            }
            
            gpiod_edge_event_buffer_free(buffer);
            return 0; // Success
        }
        
        gpiod_edge_event_buffer_free(buffer);
        return -1;
    }
#endif // GPIOD_API_VERSION

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
    line = gpiod_chip_get_line(chip, info->gpio_rel);
    if (!line) {
        fprintf(stderr, "[ERROR] Failed to get line %d on %s\n", info->gpio_rel, chip_path);
        args->success = 0;
        goto cleanup;
    }
    
    // Request line for monitoring both edges or fallback to falling only if both fails
    if (args->debug)
        fprintf(stderr, "[DEBUG] Requesting line for edge monitoring\n");
    
    ret = gpiod_line_request_both_edges_events(line, "gpio-fan-rpm");
    if (ret < 0) {
        if (args->debug)
            fprintf(stderr, "[DEBUG] Both edges failed, trying falling edge only\n");
        
        ret = gpiod_line_request_falling_edge_events(line, "gpio-fan-rpm");
        if (ret < 0) {
            fprintf(stderr, "[ERROR] Failed to request line for edge events\n");
            args->success = 0;
            goto cleanup;
        }
    }
    
    // Get file descriptor for polling
    fd = gpiod_line_event_get_fd(line);
    if (fd < 0) {
        fprintf(stderr, "[ERROR] Could not get event fd for line %d on %s\n", info->gpio_rel, chip_path);
        gpiod_line_release(line); // Release line if request succeeded but fd failed
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
            struct gpiod_line_event event;
            
            ret = gpiod_line_event_read(line, &event);
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
        gpiod_line_release(line);
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
