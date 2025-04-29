// measure_rpm_edge.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <gpiod.h> // Must be included first to check for existing definitions
#include <pthread.h>

#include "gpio-fan-rpm.h"

// Define GPIOD_LINE_EVENT_RISING_EDGE and GPIOD_LINE_EVENT_FALLING_EDGE only if not already defined
#ifndef GPIOD_LINE_EVENT_RISING_EDGE
#define GPIOD_LINE_EVENT_RISING_EDGE 1
#endif

#ifndef GPIOD_LINE_EVENT_FALLING_EDGE
#define GPIOD_LINE_EVENT_FALLING_EDGE 2
#endif

// Check if libgpiod v2 API version macro exists
#ifdef GPIOD_API_VERSION

// --- libgpiod v2 Implementation ---

static void *edge_measure_thread_v2(void *arg)
{
    edge_thread_args_t *args = (edge_thread_args_t *)arg;
    gpio_info_t *info = args->info;

    char chip_path[128];
    snprintf(chip_path, sizeof(chip_path), "/dev/%s", info->chip);

    if (args->debug)
        fprintf(stderr, "[DEBUG-v2] GPIO %d: Opening chip %s\n", info->gpio_rel, chip_path);

    struct gpiod_chip *chip = gpiod_chip_open(chip_path);
    if (!chip)
    {
        fprintf(stderr, "[ERROR-v2] Failed to open chip: %s\n", chip_path);
        args->success = 0;
        return NULL;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    struct gpiod_line_request *request = NULL; // Initialize to NULL

    if (!settings || !line_cfg || !req_cfg)
    {
        fprintf(stderr, "[ERROR-v2] Failed to allocate settings\n");
        args->success = 0;
        goto cleanup_v2;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
    // Note: Bias setting might not be available/needed in v1 fallback
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    unsigned int offset = info->gpio_rel;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
    gpiod_request_config_set_consumer(req_cfg, "gpio-fan-rpm");

    // First try to request with both edge detection (most accurate)
    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    
    // If both edges fail, try falling edge (common for tachometers)
    if (!request) {
        if (args->debug)
            fprintf(stderr, "[DEBUG-v2] GPIO %d: Both edges failed, trying falling edge...\n", info->gpio_rel);
            
        gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_FALLING);
        gpiod_line_config_reset(line_cfg);
        gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
        
        request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
        
        if (request && args->debug)
            fprintf(stderr, "[DEBUG-v2] GPIO %d: Using falling edge detection\n", info->gpio_rel);
    } else if (args->debug) {
        fprintf(stderr, "[DEBUG-v2] GPIO %d: Using both edge detection\n", info->gpio_rel);
    }
    
    if (!request)
    {
        fprintf(stderr, "[ERROR-v2] Failed to request line %d on %s\n", info->gpio_rel, chip_path);
        args->success = 0;
        goto cleanup_v2;
    }

    int fd = gpiod_line_request_get_fd(request);
    if (fd < 0)
    {
        fprintf(stderr, "[ERROR-v2] Could not get request FD for line %d on %s\n", info->gpio_rel, chip_path);
        args->success = 0;
        goto cleanup_v2;
    }

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int count = 0;
    struct pollfd pfd = {.fd = fd, .events = POLLIN};

    if (args->debug)
    {
        fprintf(stderr, "[DEBUG-v2] GPIO %d: Listening for edges (%d sec, %d pulses/rev)\n",
                info->gpio_rel, args->duration, args->pulses_per_rev);
    }

    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_sec = now.tv_sec - start.tv_sec;
        if (elapsed_sec >= args->duration)
            break;

        // Calculate remaining time for poll timeout more accurately
        long remaining_ms = (args->duration * 1000) - ((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000);
        if (remaining_ms <= 0) break; // Time's up exactly
        if (remaining_ms > 1000) remaining_ms = 1000; // Cap poll timeout

        int ret = poll(&pfd, 1, remaining_ms);
        if (ret < 0)
        {
            if (errno == EINTR) continue;
            perror("[ERROR-v2] poll()");
            break;
        }
        else if (ret == 0)
        {
            // Timeout - check if overall duration is met before continuing
            clock_gettime(CLOCK_MONOTONIC, &now);
            if ((now.tv_sec - start.tv_sec) >= args->duration) break;
            continue;
        }

        // Event occurred
        if (pfd.revents & POLLIN) {
            struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1); // Read one event at a time
            if (!buffer)
            {
                fprintf(stderr, "[ERROR-v2] Could not allocate event buffer\n");
                break;
            }

            int nread = gpiod_line_request_read_edge_events(request, buffer, 1);
            if (nread < 0) {
                 perror("[ERROR-v2] Failed to read edge events");
                 gpiod_edge_event_buffer_free(buffer); // Ensure buffer is freed on error
                 break; // Error reading
            } else if (nread > 0) {
                count += nread; // Successfully read events
            }
            // If nread == 0, it means no event was available despite poll indicating readiness,
            // which can happen. Just continue the loop after freeing the buffer.

            gpiod_edge_event_buffer_free(buffer); // Always free the buffer
        }
    }

    args->rpm_out = (count > 0 && args->pulses_per_rev > 0 && args->duration > 0)
                        ? (count * 60) / args->pulses_per_rev / args->duration
                        : 0;

    if (args->debug)
    {
        fprintf(stderr, "[DEBUG-v2] GPIO %d: Counted %d edges\n", info->gpio_rel, count);
        fprintf(stderr, "[DEBUG-v2] GPIO %d: RPM = %d (count=%d, pulses/rev=%d, duration=%d)\n",
                info->gpio_rel, args->rpm_out, count, args->pulses_per_rev, args->duration);
    }

    args->success = 1;

cleanup_v2:
    if (request) // Release request only if it was successfully created
        gpiod_line_request_release(request);
    // Settings, line_cfg, req_cfg are freed regardless of request success
    if (settings)
        gpiod_line_settings_free(settings);
    if (line_cfg)
        gpiod_line_config_free(line_cfg);
    if (req_cfg)
        gpiod_request_config_free(req_cfg);
    if (chip)
        gpiod_chip_close(chip);
    return NULL;
}

// For v2, we need to define the v1 API structures and functions that will be used
// in edge_measure_thread_v1
struct gpiod_line_event {
    struct timespec ts;
    int event_type;
};

// v1 API function prototypes for v2 environment
struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer);
int gpiod_line_event_get_fd(struct gpiod_line *line);
void gpiod_line_release(struct gpiod_line *line);
int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event);

#else // --- Fallback to libgpiod v1 API ---

#if defined(DEBUG)
static const char v1_fallback_msg[] __attribute__((unused)) = 
    "Note: Using libgpiod v1 API compatibility layer";
#endif

// Forward declare types without redefinition
struct gpiod_chip;
struct gpiod_line;

// Implementation of the v1 thread function for libgpiod v1
static void *edge_measure_thread_v1(void *arg)
{
    edge_thread_args_t *args = (edge_thread_args_t *)arg;
    gpio_info_t *info = args->info;
    
    char chip_path[128];
    snprintf(chip_path, sizeof(chip_path), "/dev/%s", info->chip);
    
    if (args->debug)
        fprintf(stderr, "[DEBUG-v1] GPIO %d: Opening chip %s\n", info->gpio_rel, chip_path);
    
    struct gpiod_chip *chip = gpiod_chip_open(chip_path);
    if (!chip)
    {
        fprintf(stderr, "[ERROR-v1] Failed to open chip: %s\n", chip_path);
        args->success = 0;
        return NULL;
    }
    
    struct gpiod_line *line = NULL;
    int fd = -1;
    int ret = -1;
    
    // Get line handle
    line = gpiod_chip_get_line(chip, info->gpio_rel);
    if (!line)
    {
        fprintf(stderr, "[ERROR-v1] Failed to get line %d on %s\n", info->gpio_rel, chip_path);
        args->success = 0;
        goto cleanup_v1;
    }
    
    // Request line for monitoring both edges or fallback to falling only if both fails
    if (args->debug)
        fprintf(stderr, "[DEBUG-v1] Requesting line for edge monitoring\n");
    
    ret = gpiod_line_request_both_edges_events(line, "gpio-fan-rpm");
    if (ret < 0)
    {
        if (args->debug)
            fprintf(stderr, "[DEBUG-v1] Both edges failed, trying falling edge only\n");
        
        ret = gpiod_line_request_falling_edge_events(line, "gpio-fan-rpm");
        if (ret < 0)
        {
            fprintf(stderr, "[ERROR-v1] Failed to request line for edge events\n");
            args->success = 0;
            goto cleanup_v1;
        }
    }
    
    // Get file descriptor for polling
    fd = gpiod_line_event_get_fd(line);
    if (fd < 0)
    {
        fprintf(stderr, "[ERROR-v1] Could not get event fd for line %d on %s\n", info->gpio_rel, chip_path);
        gpiod_line_release(line); // Release line if request succeeded but fd failed
        args->success = 0;
        goto cleanup_v1;
    }
    
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int count = 0;
    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    
    if (args->debug)
    {
        fprintf(stderr, "[DEBUG-v1] GPIO %d: Listening for edges (%d sec, %d pulses/rev)\n",
                info->gpio_rel, args->duration, args->pulses_per_rev);
    }
    
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_sec = now.tv_sec - start.tv_sec;
        if (elapsed_sec >= args->duration)
            break;
        
        // Calculate remaining time for poll timeout more accurately
        long remaining_ms = (args->duration * 1000) - ((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000);
        if (remaining_ms <= 0) break; // Time's up exactly
        if (remaining_ms > 1000) remaining_ms = 1000; // Cap poll timeout
        
        int ret = poll(&pfd, 1, remaining_ms);
        if (ret < 0)
        {
            if (errno == EINTR) continue;
            perror("[ERROR-v1] poll()");
            break;
        }
        else if (ret == 0)
        {
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
                perror("[ERROR-v1] Failed to read line event");
                break;
            }
            
            count++;
        }
    }
    
    args->rpm_out = (count > 0 && args->pulses_per_rev > 0 && args->duration > 0)
                    ? (count * 60) / args->pulses_per_rev / args->duration
                    : 0;
    
    if (args->debug)
    {
        fprintf(stderr, "[DEBUG-v1] GPIO %d: Counted %d edges\n", info->gpio_rel, count);
        fprintf(stderr, "[DEBUG-v1] GPIO %d: RPM = %d (count=%d, pulses/rev=%d, duration=%d)\n",
                info->gpio_rel, args->rpm_out, count, args->pulses_per_rev, args->duration);
    }
    
    args->success = 1;
    
cleanup_v1:
    if (line && fd >= 0)
        gpiod_line_release(line);
    if (chip)
        gpiod_chip_close(chip);
    return NULL;
}

#endif // GPIOD_API_VERSION check

// Implementation of v1 compatibility functions for v2 environment
#ifdef GPIOD_API_VERSION
// Convert v1 API calls to v2 API

struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset) {
    // In v2, lines are requested through request configs, not individually
    // This is a compatibility shim - we use a static line structure to emulate v1 behavior
    static struct {
        struct gpiod_chip *chip;
        unsigned int offset;
        int valid;
    } line_info = {0};
    
    if (chip) {
        line_info.chip = chip;
        line_info.offset = offset;
        line_info.valid = 1;
        // Return a pointer to our static struct as a "handle"
        return (struct gpiod_line *)&line_info;
    }
    return NULL;
}

int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer) {
    if (!line) return -1;
    // We don't actually make the request here - defer to when getting FD
    return 0; // Pretend it worked
}

int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer) {
    if (!line) return -1;
    // We don't actually make the request here - defer to when getting FD
    return 0; // Pretend it worked
}

// Static variables to hold v2 request data
static struct gpiod_line_settings *v1_compat_settings = NULL;
static struct gpiod_line_config *v1_compat_line_cfg = NULL;
static struct gpiod_request_config *v1_compat_req_cfg = NULL;
static struct gpiod_line_request *v1_compat_request = NULL;

int gpiod_line_event_get_fd(struct gpiod_line *line) {
    if (!line) return -1;
    
    // Extract the line_info we stored in gpiod_chip_get_line
    struct {
        struct gpiod_chip *chip;
        unsigned int offset;
        int valid;
    } *line_info = (void *)line;
    
    if (!line_info->valid || !line_info->chip) return -1;
    
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
    if (!line) return;
    
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
    if (!line || !event || !v1_compat_request) return -1;
    
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
#endif

// --- Common measure_rpm_edge function ---

int measure_rpm_edge(gpio_info_t *infos, int count, int duration, int debug)
{
    pthread_t threads[MAX_GPIOS];
    edge_thread_args_t args[MAX_GPIOS];

    if (count > MAX_GPIOS) {
        fprintf(stderr, "[ERROR] Exceeded maximum GPIO count (%d)\n", MAX_GPIOS);
        return -1;
    }

#ifdef GPIOD_API_VERSION
    if (debug) fprintf(stderr, "[DEBUG] Using libgpiod v2 API for edge measurement\n");
    void *(*thread_func)(void *) = edge_measure_thread_v2;
#else
    if (debug) fprintf(stderr, "[DEBUG] Using libgpiod v1 API fallback for edge measurement\n");
    void *(*thread_func)(void *) = edge_measure_thread_v1;
#endif

    if (debug)
    {
        fprintf(stderr, "[DEBUG] Measurement duration: %d second(s)\n", duration);
    }

    int threads_created = 0;
    for (int i = 0; i < count; i++)
    {
        args[i].info = &infos[i];
        args[i].duration = duration;
        args[i].pulses_per_rev = infos[i].pulses_per_rev; // Use per-GPIO pulses
        args[i].debug = debug;
        args[i].rpm_out = 0;
        args[i].success = 0; // Assume failure until thread completes successfully

        if (debug)
        {
            fprintf(stderr, "[DEBUG] Preparing thread for GPIO %d (chip=%s, pulses/rev=%d)\n",
                    infos[i].gpio_rel, infos[i].chip, args[i].pulses_per_rev);
        }

        if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0)
        {
            fprintf(stderr, "[ERROR] Failed to create thread for GPIO %d: %s\n", infos[i].gpio_rel, strerror(errno));
            // Don't immediately return -1. Try to clean up already created threads.
            count = i; // Only join threads up to this point
            goto join_threads; // Jump to cleanup
        }
        threads_created++;
    }

join_threads:
    for (int i = 0; i < threads_created; i++) // Only join threads that were successfully created
    {
        void *thread_ret = NULL;
        int join_ret = pthread_join(threads[i], &thread_ret);
        
        if (join_ret != 0) {
            if (debug) fprintf(stderr, "[DEBUG] Thread join error for GPIO %d: %s\n", 
                               infos[i].gpio_rel, strerror(join_ret));
        }
        
        // Update info based on thread result
        if (args[i].success)
        {
            infos[i].rpm = args[i].rpm_out;
            infos[i].valid = 1;
        }
        else
        {
            // Keep previous valid state if measurement failed? Or mark invalid? Mark invalid.
            infos[i].rpm = 0; // Reset RPM on failure
            infos[i].valid = 0;
            if (debug) fprintf(stderr, "[DEBUG] Thread for GPIO %d failed or did not complete successfully.\n", infos[i].gpio_rel);
        }
    }

    // If thread creation failed for some GPIOs, return an error code
    if (threads_created < count) {
        return -1;
    }

    return 0; // Success
}
