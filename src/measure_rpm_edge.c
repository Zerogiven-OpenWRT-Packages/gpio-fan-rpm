// measure_rpm_edge.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <gpiod.h> // Must be included before checking GPIOD_API_VERSION
#include <pthread.h>

#include "gpio-fan-rpm.h"

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

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
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
                 gpiod_edge_event_buffer_free(buffer);
                 break; // Error reading
            } else if (nread > 0) {
                count += nread; // Successfully read events
            }
            // If nread == 0, it means no event was available despite poll indicating readiness,
            // which can happen. Just continue the loop.

            gpiod_edge_event_buffer_free(buffer);
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

#else // --- Fallback to libgpiod v1 API ---

#warning "Compiling with libgpiod v1 API fallback. GPIO bias settings might not be applied."

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

    line = gpiod_chip_get_line(chip, info->gpio_rel);
    if (!line) {
        fprintf(stderr, "[ERROR-v1] Failed to get line %d on %s: %s\n", info->gpio_rel, chip_path, strerror(errno));
        gpiod_chip_close(chip);
        args->success = 0;
        return NULL;
    }

    // Request edge events using v1 API
    // Try both edges first, then fallback to falling (common for tachometers)
    // Note: v1 API doesn't directly support setting bias (like pull-up) during request.
    // This relies on kernel default or hardware configuration.
    ret = gpiod_line_request_both_edges_events(line, "gpio-fan-rpm");
    if (ret < 0) {
        if (args->debug) fprintf(stderr, "[DEBUG-v1] GPIO %d: Both edges failed, trying falling edge...\n", info->gpio_rel);
        ret = gpiod_line_request_falling_edge_events(line, "gpio-fan-rpm");
        if (ret < 0) {
             fprintf(stderr, "[ERROR-v1] Failed to request edge events for line %d on %s: %s\n", info->gpio_rel, chip_path, strerror(errno));
             // No need to release line if request failed
             gpiod_chip_close(chip);
             args->success = 0;
             return NULL;
        }
         if (args->debug) fprintf(stderr, "[DEBUG-v1] GPIO %d: Using falling edge detection\n", info->gpio_rel);
    } else {
         if (args->debug) fprintf(stderr, "[DEBUG-v1] GPIO %d: Using both edge detection\n", info->gpio_rel);
    }


    fd = gpiod_line_event_get_fd(line);
    if (fd < 0) {
        fprintf(stderr, "[ERROR-v1] Failed to get event fd for line %d on %s: %s\n", info->gpio_rel, chip_path, strerror(errno));
        gpiod_line_release(line); // Release line if request succeeded but fd failed
        gpiod_chip_close(chip);
        args->success = 0;
        return NULL;
    }

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int count = 0;

    if (args->debug) {
        fprintf(stderr, "[DEBUG-v1] GPIO %d: Listening for edges (%d sec, %d pulses/rev)\n",
                info->gpio_rel, args->duration, args->pulses_per_rev);
    }

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_sec = now.tv_sec - start.tv_sec;
        if (elapsed_sec >= args->duration) {
            break; // Measurement duration elapsed
        }

        // Calculate remaining time for poll timeout more accurately
        long remaining_ms = (args->duration * 1000) - ((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000);
        if (remaining_ms <= 0) break; // Time's up exactly
        if (remaining_ms > 1000) remaining_ms = 1000; // Cap poll timeout


        struct pollfd pfd = {.fd = fd, .events = POLLIN};
        ret = poll(&pfd, 1, remaining_ms); // Poll with timeout

        if (ret < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, try again
            fprintf(stderr, "[ERROR-v1] poll() failed for line %d on %s: %s\n", info->gpio_rel, chip_path, strerror(errno));
            break; // Unrecoverable poll error
        } else if (ret == 0) {
            // Timeout - check if overall duration is met before continuing
             clock_gettime(CLOCK_MONOTONIC, &now);
             if ((now.tv_sec - start.tv_sec) >= args->duration) break;
            continue; // No event within timeout, continue loop
        }

        // Event occurred, read it
        if (pfd.revents & POLLIN) {
            struct gpiod_line_event event;
            // Read one event at a time
            ret = gpiod_line_event_read(line, &event);
            if (ret == 0) {
                count++; // Successfully read an event
            } else if (ret < 0) {
                 // EAGAIN might mean no event ready despite poll, EINTR is recoverable
                 if (errno == EAGAIN || errno == EINTR) {
                     if (args->debug) fprintf(stderr, "[DEBUG-v1] GPIO %d: gpiod_line_event_read non-fatal error: %s\n", info->gpio_rel, strerror(errno));
                     continue;
                 }
                 fprintf(stderr, "[ERROR-v1] Failed to read event for line %d on %s: %s\n", info->gpio_rel, chip_path, strerror(errno));
                 break; // Unrecoverable read error
            }
            // If ret > 0 in v1, it's an error code, not number of events read.
        }
    }

    // Cleanup
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    args->rpm_out = (count > 0 && args->pulses_per_rev > 0 && args->duration > 0)
                        ? (count * 60) / args->pulses_per_rev / args->duration
                        : 0;

     if (args->debug) {
        fprintf(stderr, "[DEBUG-v1] GPIO %d: Counted %d edges\n", info->gpio_rel, count);
        fprintf(stderr, "[DEBUG-v1] GPIO %d: RPM = %d (count=%d, pulses/rev=%d, duration=%d)\n",
                info->gpio_rel, args->rpm_out, count, args->pulses_per_rev, args->duration);
    }

    args->success = 1;
    return NULL;
}

#endif // GPIOD_API_VERSION check


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
