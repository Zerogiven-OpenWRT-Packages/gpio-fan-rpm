#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <gpiod.h>
#include "gpio_compat.h"

// This file is specifically for v2 API (OpenWRT 24.10)

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

// Get a line handle from a chip
struct gpiod_line *gpio_compat_get_line(struct gpiod_chip *chip, unsigned int offset)
{
    if (!chip) return NULL;
    
    line_storage.chip = chip;
    line_storage.offset = offset;
    line_storage.valid = 1;
    
    return (struct gpiod_line *)&line_storage;
}

// Request edge events on a line
int gpio_compat_request_events(struct gpiod_line *line, const char *consumer, int falling_only)
{
    // In v2, we defer the actual request until get_fd is called
    return 0;
}

// Get a file descriptor for polling
int gpio_compat_get_fd(struct gpiod_line *line)
{
    if (!line) return -1;
    
    // Extract the line info struct we created in gpio_compat_get_line
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
}

// Release resources associated with a line
void gpio_compat_release_line(struct gpiod_line *line)
{
    if (!line) return;

    // Clean up v2 resources
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
}

// Read an event from a line
int gpio_compat_read_event(struct gpiod_line *line, struct gpio_compat_event *event)
{
    if (!line || !event || !compat_request) return -1;
    
    // Read edge events using v2 API
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
}

// Get the path for a chip
const char *gpio_compat_get_chip_path(struct gpiod_chip *chip)
{
    if (!chip) return NULL;
    
    static char path[128];
    const char *name = gpiod_chip_get_name(chip);
    
    snprintf(path, sizeof(path), "/dev/%s", name ? name : "gpiochip?");
    return path;
}