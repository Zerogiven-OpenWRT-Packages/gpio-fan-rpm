#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "gpio_compat.h"

// Check if we're using libgpiod v2 API
#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // We're in v2 API mode
    #define USING_GPIOD_V2 1
    
    // Static variables for v2 compatibility
    static struct gpiod_line_settings *compat_settings = NULL;
    static struct gpiod_line_config *compat_line_cfg = NULL;
    static struct gpiod_request_config *compat_req_cfg = NULL;
    static struct gpiod_line_request *compat_request = NULL;
    
    // Line info storage for our v2 implementation
    typedef struct {
        struct gpiod_chip *chip;
        unsigned int offset;
        int valid;
    } compat_line_info_t;
    
    static compat_line_info_t line_storage = {0};
#endif

struct gpiod_line *gpio_compat_get_line(struct gpiod_chip *chip, unsigned int offset)
{
    if (!chip) return NULL;

#ifdef USING_GPIOD_V2
    // v2 API implementation
    line_storage.chip = chip;
    line_storage.offset = offset;
    line_storage.valid = 1;
    return (struct gpiod_line *)&line_storage;
#else
    // v1 API implementation
    return gpiod_chip_get_line(chip, offset);
#endif
}

int gpio_compat_request_events(struct gpiod_line *line, const char *consumer, int falling_only)
{
    if (!line) return -1;

#ifdef USING_GPIOD_V2
    // In v2, we defer the actual request until get_fd is called
    return 0;
#else
    // v1 API implementation
    if (falling_only) {
        return gpiod_line_request_falling_edge_events(line, consumer);
    } else {
        return gpiod_line_request_both_edges_events(line, consumer);
    }
#endif
}

int gpio_compat_get_fd(struct gpiod_line *line)
{
    if (!line) return -1;

#ifdef USING_GPIOD_V2
    // v2 API implementation - now we set up and make the actual request
    compat_line_info_t *info = (compat_line_info_t *)line;
    if (!info->valid || !info->chip) return -1;
    
    // Clean up any existing request resources
    if (compat_request) {
        gpiod_line_request_release(compat_request);
        compat_request = NULL;
    }
    
    // Set up a new request with the right settings
    compat_settings = gpiod_line_settings_new();
    if (!compat_settings) return -1;
    
    gpiod_line_settings_set_direction(compat_settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(compat_settings, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_bias(compat_settings, GPIOD_LINE_BIAS_PULL_UP);
    
    compat_line_cfg = gpiod_line_config_new();
    if (!compat_line_cfg) {
        gpiod_line_settings_free(compat_settings);
        compat_settings = NULL;
        return -1;
    }
    
    compat_req_cfg = gpiod_request_config_new();
    if (!compat_req_cfg) {
        gpiod_line_settings_free(compat_settings);
        gpiod_line_config_free(compat_line_cfg);
        compat_settings = NULL;
        compat_line_cfg = NULL;
        return -1;
    }
    
    // Configure and make the request
    unsigned int offset = info->offset;
    gpiod_line_config_add_line_settings(compat_line_cfg, &offset, 1, compat_settings);
    gpiod_request_config_set_consumer(compat_req_cfg, "gpio-fan-rpm");
    
    compat_request = gpiod_chip_request_lines(info->chip, compat_req_cfg, compat_line_cfg);
    if (!compat_request) {
        gpiod_line_settings_free(compat_settings);
        gpiod_line_config_free(compat_line_cfg);
        gpiod_request_config_free(compat_req_cfg);
        compat_settings = NULL;
        compat_line_cfg = NULL;
        compat_req_cfg = NULL;
        return -1;
    }
    
    return gpiod_line_request_get_fd(compat_request);
#else
    // v1 API implementation
    return gpiod_line_event_get_fd(line);
#endif
}

void gpio_compat_release_line(struct gpiod_line *line)
{
    if (!line) return;

#ifdef USING_GPIOD_V2
    // v2 API - clean up resources
    if (compat_request) {
        gpiod_line_request_release(compat_request);
        compat_request = NULL;
    }
    if (compat_settings) {
        gpiod_line_settings_free(compat_settings);
        compat_settings = NULL;
    }
    if (compat_line_cfg) {
        gpiod_line_config_free(compat_line_cfg);
        compat_line_cfg = NULL;
    }
    if (compat_req_cfg) {
        gpiod_request_config_free(compat_req_cfg);
        compat_req_cfg = NULL;
    }
#else
    // v1 API implementation
    gpiod_line_release(line);
#endif
}

int gpio_compat_read_event(struct gpiod_line *line, struct gpio_compat_event *event)
{
    if (!line || !event) return -1;

#ifdef USING_GPIOD_V2
    // v2 API implementation
    if (!compat_request) return -1;
    
    // Read events using v2 API
    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
    if (!buffer) return -1;
    
    int ret = gpiod_line_request_read_edge_events(compat_request, buffer, 1);
    if (ret <= 0) {
        gpiod_edge_event_buffer_free(buffer);
        return (ret < 0) ? ret : -1;
    }
    
    // Convert to our event format
    const struct gpiod_edge_event *v2_event = gpiod_edge_event_buffer_get_event(buffer, 0);
    if (v2_event) {
        event->ts = gpiod_edge_event_get_timestamp(v2_event);
        
        int event_type = gpiod_edge_event_get_event_type(v2_event);
        if (event_type == GPIOD_EDGE_EVENT_RISING_EDGE) {
            event->event_type = GPIO_COMPAT_RISING_EDGE;
        } else {
            event->event_type = GPIO_COMPAT_FALLING_EDGE;
        }
        
        gpiod_edge_event_buffer_free(buffer);
        return 0; // Success
    }
    
    gpiod_edge_event_buffer_free(buffer);
    return -1;
#else
    // v1 API implementation - read from fd directly to avoid struct definition issues
    int fd = gpio_compat_get_fd(line);
    if (fd < 0) return -1;
    
    // Using the actual gpiod_line_event struct which exists in v1
    struct gpiod_line_event v1_event;
    int ret = read(fd, &v1_event, sizeof(v1_event));
    
    if (ret <= 0) return (ret < 0) ? ret : -1;
    
    // Copy to our common structure
    event->ts = v1_event.ts;
    
    // Convert event type to our constants
    if (v1_event.event_type == 1) { // GPIOD_LINE_EVENT_RISING_EDGE in v1
        event->event_type = GPIO_COMPAT_RISING_EDGE;
    } else {
        event->event_type = GPIO_COMPAT_FALLING_EDGE;
    }
    
    return 0;
#endif
}

const char *gpio_compat_get_chip_path(struct gpiod_chip *chip)
{
    if (!chip) return NULL;
    
    static char path[128];

#ifdef USING_GPIOD_V2
    // v2 API implementation
    snprintf(path, sizeof(path), "/dev/%s", gpiod_chip_get_name(chip));
    return path;
#else
    // v1 API implementation - use system function if available or fallback
    #ifdef gpiod_chip_get_path
    return gpiod_chip_get_path(chip);
    #else
    const char *name = gpiod_chip_get_name(chip);
    if (name) {
        snprintf(path, sizeof(path), "/dev/%s", name);
    } else {
        snprintf(path, sizeof(path), "/dev/gpiochip?");
    }
    return path;
    #endif
#endif
}