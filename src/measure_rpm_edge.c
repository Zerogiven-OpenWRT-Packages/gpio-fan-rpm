// measure_rpm_edge.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <gpiod.h>
#include <pthread.h>

#include "gpio-fan-rpm.h"

static void *edge_measure_thread(void *arg)
{
    edge_thread_args_t *args = (edge_thread_args_t *)arg;
    gpio_info_t *info = args->info;

    char chip_path[128];
    snprintf(chip_path, sizeof(chip_path), "/dev/%s", info->chip);

    if (args->debug)
        fprintf(stderr, "[DEBUG] GPIO %d: Opening chip %s\n", info->gpio_rel, chip_path);

    struct gpiod_chip *chip = gpiod_chip_open(chip_path);
    if (!chip)
    {
        fprintf(stderr, "[ERROR] Failed to open chip: %s\n", chip_path);
        args->success = 0;
        return NULL;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();

    if (!settings || !line_cfg || !req_cfg)
    {
        fprintf(stderr, "[ERROR] Failed to allocate settings\n");
        args->success = 0;
        goto cleanup;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    unsigned int offset = info->gpio_rel;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
    gpiod_request_config_set_consumer(req_cfg, "gpio-fan-rpm");

    struct gpiod_line_request *request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request)
    {
        fprintf(stderr, "[ERROR] Failed to request line\n");
        args->success = 0;
        goto cleanup;
    }

    int fd = gpiod_line_request_get_fd(request);
    if (fd < 0)
    {
        fprintf(stderr, "[ERROR] Could not get request FD\n");
        args->success = 0;
        goto cleanup;
    }

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int count = 0;
    struct pollfd pfd = {.fd = fd, .events = POLLIN};

    if (args->debug)
    {
        fprintf(stderr, "[DEBUG] GPIO %d: Listening for edges (%d sec, %d pulses/rev)\n",
                info->gpio_rel, args->duration, args->pulses_per_rev);
    }

    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &now);
        if ((now.tv_sec - start.tv_sec) >= args->duration)
            break;

        int ret = poll(&pfd, 1, 1000); // 200ms timeout
        if (ret < 0)
        {
            if (errno == EINTR) continue;
            perror("[ERROR] poll()");
            break;
        }
        else if (ret == 0)
        {
            // Timeout - no event, continue
            continue;
        }

        struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
        if (!buffer)
        {
            fprintf(stderr, "[ERROR] Could not allocate event buffer\n");
            break;
        }

        int nread = gpiod_line_request_read_edge_events(request, buffer, 1);
        if (nread > 0)
            count += nread;

        gpiod_edge_event_buffer_free(buffer);
    }

    gpiod_line_request_release(request);

    args->rpm_out = (count > 0)
                        ? (count * 60) / args->pulses_per_rev / args->duration
                        : 0;

    if (args->debug)
    {
        fprintf(stderr, "[DEBUG] GPIO %d: Counted %d edges\n", info->gpio_rel, count);
        fprintf(stderr, "[DEBUG] GPIO %d: RPM = %d (count=%d, pulses/rev=%d, duration=%d)\n",
                info->gpio_rel, args->rpm_out, count, args->pulses_per_rev, args->duration);
    }

    args->success = 1;

cleanup:
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

int measure_rpm_edge(gpio_info_t *infos, int count, int duration, int debug)
{
    pthread_t threads[MAX_GPIOS];
    edge_thread_args_t args[MAX_GPIOS];

    if (debug)
    {
        fprintf(stderr, "[DEBUG] Duration: %d second(s)\n", duration);
    }

    for (int i = 0; i < count; i++)
    {
        args[i].info = &infos[i];
        args[i].duration = duration;
        args[i].pulses_per_rev = infos[i].pulses_per_rev;
        args[i].debug = debug;
        args[i].rpm_out = 0;
        args[i].success = 0;

        if (debug)
        {
            fprintf(stderr, "[DEBUG] GPIO %d → chip=%s, pulses/rev=%d\n",
                    infos[i].gpio_rel, infos[i].chip, infos[i].pulses_per_rev);
        }

        if (pthread_create(&threads[i], NULL, edge_measure_thread, &args[i]) != 0)
        {
            fprintf(stderr, "[ERROR] Failed to create thread for GPIO %d\n", infos[i].gpio_rel);
            return -1;
        }
    }

    for (int i = 0; i < count; i++)
    {
        pthread_join(threads[i], NULL);
        if (args[i].success)
        {
            infos[i].rpm = args[i].rpm_out;
            infos[i].valid = 1;
        }
        else
        {
            infos[i].valid = 0;
        }
    }

    return 0;
}
