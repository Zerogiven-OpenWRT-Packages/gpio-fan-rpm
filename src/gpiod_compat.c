//
// gpiod_compat.c - Compatibility layer for libgpiod v1/v2
//
// This file provides compatibility functions to bridge the gap between
// libgpiod v1 (OpenWRT 23.05) and v2 (OpenWRT 24.10)
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "gpiod_compat.h" // Include our compat header first

// Implementation of compatibility functions depending on which API version we're using
#ifdef USING_LIBGPIOD_V2
// This file implements v1-style API functions when compiled with libgpiod v2

// This is a cache for line requests to emulate v1 API with v2 API
#define MAX_LINE_CACHE 32

typedef struct {
    struct gpiod_line *line_ptr;
    struct gpiod_line_request *request;
    int fd;
    int in_use;
} line_cache_t;

static line_cache_t line_cache[MAX_LINE_CACHE] = {0};

// Helper to find or allocate a line cache entry
static line_cache_t *get_line_cache(struct gpiod_line *line) {
    // First look for an existing entry for this line
    for (int i = 0; i < MAX_LINE_CACHE; i++) {
        if (line_cache[i].in_use && line_cache[i].line_ptr == line) {
            return &line_cache[i];
        }
    }

    // Not found, allocate a new one
    for (int i = 0; i < MAX_LINE_CACHE; i++) {
        if (!line_cache[i].in_use) {
            line_cache[i].line_ptr = line;
            line_cache[i].in_use = 1;
            return &line_cache[i];
        }
    }

    // No free slots
    errno = ENOMEM;
    return NULL;
}

// Emulate gpiod_chip_get_line from v1 with v2
struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset) {
    if (!chip) {
        errno = EINVAL;
        return NULL;
    }

    // In v2, we don't have direct line handles, so we create a "fake" handle
    static struct {
        struct gpiod_chip *chip;
        unsigned int offset;
    } line_handles[MAX_LINE_CACHE] = {0};

    // Find a free slot
    for (int i = 0; i < MAX_LINE_CACHE; i++) {
        if (line_handles[i].chip == NULL) {
            line_handles[i].chip = chip;
            line_handles[i].offset = offset;
            return (struct gpiod_line *)&line_handles[i];
        }
    }

    errno = ENOMEM;
    return NULL;
}

// Common function to request line edges
static int request_line_events(struct gpiod_line *line, const char *consumer, int edge_type) {
    if (!line || !consumer) {
        errno = EINVAL;
        return -1;
    }

    // Extract chip and offset from our fake line handle
    struct {
        struct gpiod_chip *chip;
        unsigned int offset;
    } *line_handle = (void *)line;

    struct gpiod_chip *chip = line_handle->chip;
    unsigned int offset = line_handle->offset;

    // Set up line request with v2 API
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    struct gpiod_line_request *request = NULL;

    if (!settings || !line_cfg || !req_cfg) {
        errno = ENOMEM;
        goto cleanup;
    }

    // Configure for edge detection
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, edge_type);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
    gpiod_request_config_set_consumer(req_cfg, consumer);

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    
    // Clean up configuration objects
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
    
    if (!request) {
        // Try without pull-up if it failed
        settings = gpiod_line_settings_new();
        line_cfg = gpiod_line_config_new();
        req_cfg = gpiod_request_config_new();
        
        if (!settings || !line_cfg || !req_cfg) {
            errno = ENOMEM;
            goto cleanup;
        }
        
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
        gpiod_line_settings_set_edge_detection(settings, edge_type);
        
        gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
        gpiod_request_config_set_consumer(req_cfg, consumer);
        
        request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
        
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_request_config_free(req_cfg);
        
        if (!request) {
            return -1;
        }
    }

    // Store the request in our cache
    line_cache_t *cache = get_line_cache(line);
    if (!cache) {
        gpiod_line_request_release(request);
        return -1;
    }

    // Get the file descriptor
    int fd = gpiod_line_request_get_fd(request);
    if (fd < 0) {
        gpiod_line_request_release(request);
        return -1;
    }

    cache->request = request;
    cache->fd = fd;
    
    return 0;

cleanup:
    if (settings) gpiod_line_settings_free(settings);
    if (line_cfg) gpiod_line_config_free(line_cfg);
    if (req_cfg) gpiod_request_config_free(req_cfg);
    return -1;
}

// v1 API: Request both edges
int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer) {
    return request_line_events(line, consumer, GPIOD_LINE_EDGE_BOTH);
}

// v1 API: Request falling edge
int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer) {
    return request_line_events(line, consumer, GPIOD_LINE_EDGE_FALLING);
}

// v1 API: Get file descriptor
int gpiod_line_event_get_fd(struct gpiod_line *line) {
    if (!line) {
        errno = EINVAL;
        return -1;
    }

    line_cache_t *cache = get_line_cache(line);
    if (!cache || !cache->request) {
        errno = ENOENT;
        return -1;
    }

    return cache->fd;
}

// v1 API: Read event
int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event) {
    if (!line || !event) {
        errno = EINVAL;
        return -1;
    }

    line_cache_t *cache = get_line_cache(line);
    if (!cache || !cache->request) {
        errno = ENOENT;
        return -1;
    }

    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
    if (!buffer) {
        errno = ENOMEM;
        return -1;
    }

    int ret = gpiod_line_request_read_edge_events(cache->request, buffer, 1);
    if (ret < 0) {
        gpiod_edge_event_buffer_free(buffer);
        return -1;
    }

    if (ret == 0) {
        // No events available
        gpiod_edge_event_buffer_free(buffer);
        errno = EAGAIN;
        return -1;
    }

    // Get the first event from the buffer
    struct gpiod_edge_event *v2_event = gpiod_edge_event_buffer_get_event(buffer, 0);
    if (!v2_event) {
        gpiod_edge_event_buffer_free(buffer);
        errno = EINVAL;
        return -1;
    }

    // Copy timestamp
    event->ts = gpiod_edge_event_get_timestamp(v2_event);

    // Convert event type
    int v2_event_type = gpiod_edge_event_get_event_type(v2_event);
    if (v2_event_type == GPIOD_EDGE_EVENT_RISING_EDGE) {
        event->event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    } else if (v2_event_type == GPIOD_EDGE_EVENT_FALLING_EDGE) {
        event->event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
    } else {
        // Unknown event type
        gpiod_edge_event_buffer_free(buffer);
        errno = EINVAL;
        return -1;
    }

    gpiod_edge_event_buffer_free(buffer);
    return 0; // Success
}

// v1 API: Release line
void gpiod_line_release(struct gpiod_line *line) {
    if (!line) return;

    line_cache_t *cache = get_line_cache(line);
    if (!cache || !cache->request) {
        return;
    }

    gpiod_line_request_release(cache->request);
    cache->request = NULL;
    cache->fd = -1;
    cache->in_use = 0;
    
    // Clean up the fake line handle
    struct {
        struct gpiod_chip *chip;
        unsigned int offset;
    } *line_handle = (void *)line;
    
    line_handle->chip = NULL;
}

// v1 API: Get value
int gpiod_line_get_value(struct gpiod_line *line) {
    if (!line) {
        errno = EINVAL;
        return -1;
    }

    // Extract chip and offset from our fake line handle
    struct {
        struct gpiod_chip *chip;
        unsigned int offset;
    } *line_handle = (void *)line;

    struct gpiod_chip *chip = line_handle->chip;
    unsigned int offset = line_handle->offset;

    // Set up a temporary line request to read the value
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    struct gpiod_line_request *request = NULL;
    int value = -1;

    if (!settings || !line_cfg || !req_cfg) {
        errno = ENOMEM;
        goto cleanup;
    }

    // Configure for input
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
    gpiod_request_config_set_consumer(req_cfg, "gpiod_compat_get_value");

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request) {
        goto cleanup;
    }

    // Read the value
    value = gpiod_line_request_get_value(request, 0);

    // Clean up
    gpiod_line_request_release(request);

cleanup:
    if (settings) gpiod_line_settings_free(settings);
    if (line_cfg) gpiod_line_config_free(line_cfg);
    if (req_cfg) gpiod_request_config_free(req_cfg);
    
    return value;
}

#endif // USING_LIBGPIOD_V2