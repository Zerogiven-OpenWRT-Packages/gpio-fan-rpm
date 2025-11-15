/**
 * @file line.c
 * @brief GPIO line operations implementation
 * @author CSoellinger
 * @date 2025
 * @license LGPL-3.0-or-later
 * 
 * This module implements GPIO line operations with compatibility for both
 * libgpiod v1 and v2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "line.h"

line_request_t* line_request_events(struct gpiod_chip *chip, int gpio, const char *consumer) {
    if (!chip || !consumer) return NULL;
    
    line_request_t *req = calloc(1, sizeof(*req));
    if (!req) return NULL;
    
    req->gpio = gpio;
    
#ifdef LIBGPIOD_V2
    // Create request configuration
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    if (!req_cfg) {
        free(req);
        return NULL;
    }
    
    gpiod_request_config_set_consumer(req_cfg, consumer);
    
    // Create line configuration
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }
    
    // Configure line for edge detection
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        gpiod_line_config_free(line_cfg);
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }
    
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
    // Note: Clock setting is not needed for basic edge detection
    
    // Add line to configuration
    unsigned int gpio_offset = (unsigned int)gpio;
    if (gpiod_line_config_add_line_settings(line_cfg, &gpio_offset, 1, settings) < 0) {
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }
    
    // Request the line
    req->request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!req->request) {
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_request_config_free(req_cfg);
        free(req);
        return NULL;
    }
    
    // Get event file descriptor
    req->event_fd = gpiod_line_request_get_fd(req->request);
    
    // Clean up configuration objects
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
    
#else
    // For libgpiod v1
    req->line = gpiod_chip_get_line(chip, gpio);
    if (!req->line) {
        free(req);
        return NULL;
    }
    
    // Request both edge events - try rising first, then falling
    int ret1 = gpiod_line_request_rising_edge_events(req->line, consumer);
    int ret2 = gpiod_line_request_falling_edge_events(req->line, consumer);
    
    // For v1, we need to request both separately, but either one should work
    if (ret1 < 0 && ret2 < 0) {
        gpiod_line_release(req->line);
        free(req);
        return NULL;
    }
#endif
    
    return req;
}

void line_release(line_request_t *req) {
    if (!req) return;
    
#ifdef LIBGPIOD_V2
    if (req->request) {
        gpiod_line_request_release(req->request);
    }
#else
    if (req->line) {
        gpiod_line_release(req->line);
    }
#endif
    
    free(req);
}

int line_wait_event(line_request_t *req, int64_t timeout_ns) {
    if (!req) return -1;
    
#ifdef LIBGPIOD_V2
    if (req->event_fd < 0) return -1;
    
    struct pollfd pfd;
    pfd.fd = req->event_fd;
    pfd.events = POLLIN;
    
    int timeout_ms = (timeout_ns >= 0) ? (timeout_ns / 1000000LL) : -1;
    int ret = poll(&pfd, 1, timeout_ms);
    
    if (ret < 0) return -1;  // Error
    if (ret == 0) return 0;  // Timeout
    return 1;  // Event available
#else
    if (!req->line) return -1;
    
    struct timespec ts;
    ts.tv_sec = timeout_ns / 1000000000LL;
    ts.tv_nsec = timeout_ns % 1000000000LL;
    
    return gpiod_line_event_wait(req->line, &ts);
#endif
}

int line_read_event(line_request_t *req) {
    if (!req) return -1;
    
#ifdef LIBGPIOD_V2
    if (!req->request) return -1;
    
    // For libgpiod v2, we need to use an event buffer
    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
    if (!buffer) return -1;
    
    int ret = gpiod_line_request_read_edge_events(req->request, buffer, 1);
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
    if (!req->line) return -1;
    
    struct gpiod_line_event event;
    return gpiod_line_event_read(req->line, &event);
#endif
} 