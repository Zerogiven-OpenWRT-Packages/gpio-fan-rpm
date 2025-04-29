//
// gpiod_compat.c - Compatibility layer for libgpiod v1/v2
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "gpiod_compat.h"
#include "gpio-fan-rpm.h"

// Check if we're using libgpiod v2 API
#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // Static variables for v2 compatibility
    static struct gpiod_line_settings *v1_compat_settings = NULL;
    static struct gpiod_line_config *v1_compat_line_cfg = NULL;
    static struct gpiod_request_config *v1_compat_req_cfg = NULL;
    static struct gpiod_line_request *v1_compat_request = NULL;
#endif

// Implementations for the compatibility functions
struct gpiod_line *compat_gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset)
{
    if (!chip) return NULL;

#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // v2 API implementation
    static struct {
        struct gpiod_chip *chip;
        unsigned int offset;
        int valid;
    } line_info = {0};
    
    line_info.chip = chip;
    line_info.offset = offset;
    line_info.valid = 1;
    return (struct gpiod_line *)&line_info;
#else
    // v1 API - direct call to the system function
    return gpiod_chip_get_line(chip, offset);
#endif
}

int compat_gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer)
{
    if (!line) return -1;

#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // v2 API implementation - defer to event_get_fd
    return 0; // Pretend it worked
#else
    // v1 API - direct call to the system function
    return gpiod_line_request_both_edges_events(line, consumer);
#endif
}

int compat_gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer)
{
    if (!line) return -1;

#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // v2 API implementation - defer to event_get_fd
    return 0; // Pretend it worked
#else
    // v1 API - direct call to the system function
    return gpiod_line_request_falling_edge_events(line, consumer);
#endif
}

int compat_gpiod_line_event_get_fd(struct gpiod_line *line)
{
    if (!line) return -1;

#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // v2 API implementation
    // Extract the line_info we stored in compat_gpiod_chip_get_line
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
    
    gpiod_line_settings_set_direction(v1_compat_settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(v1_compat_settings, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_bias(v1_compat_settings, GPIOD_LINE_BIAS_PULL_UP);
    
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
#else
    // v1 API - direct call to the system function
    return gpiod_line_event_get_fd(line);
#endif
}

void compat_gpiod_line_release(struct gpiod_line *line)
{
    if (!line) return;

#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // v2 API implementation
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
#else
    // v1 API - direct call to the system function
    gpiod_line_release(line);
#endif
}

int compat_gpiod_line_event_read(struct gpiod_line *line, struct compat_gpiod_line_event *event)
{
    if (!line || !event) return -1;

#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // v2 API implementation
    if (!v1_compat_request) return -1;
    
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
#else
    // v1 API implementation using our compat structure
    // We need to use a temporary struct that matches the system's definition
    int ret;
    // Use the raw read syscall on the file descriptor instead 
    // to avoid the struct definition problem
    int fd = compat_gpiod_line_event_get_fd(line);
    if (fd < 0) return -1;
    
    // Read the raw event data directly from the file descriptor
    // The v1 API struct has timestamp (16 bytes) and event_type (4 bytes)
    unsigned char raw_event[24] = {0}; // Oversized to be safe
    ret = read(fd, raw_event, sizeof(raw_event));
    
    if (ret <= 0) return (ret < 0) ? ret : -1;
    
    // Parse the raw event data - this assumes little-endian
    // First 16 bytes are timestamp (struct timespec)
    memcpy(&event->ts, raw_event, sizeof(struct timespec));
    
    // Next 4 bytes should be the event type
    int event_type;
    memcpy(&event_type, raw_event + sizeof(struct timespec), sizeof(int));
    event->event_type = event_type;
    
    return 0;
#endif
}

const char *compat_gpiod_chip_get_path(struct gpiod_chip *chip)
{
    if (!chip) return NULL;

#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // v2 API implementation
    static char path[128];
    snprintf(path, sizeof(path), "/dev/%s", gpiod_chip_get_name(chip));
    return path;
#else
    // v1 API implementation - try to use direct calls
    static char path[128];
    
    #ifdef gpiod_chip_get_path
    return gpiod_chip_get_path(chip);
    #else
    // Fallback implementation
    #ifdef gpiod_chip_get_name
    const char *name = gpiod_chip_get_name(chip);
    if (name) {
        snprintf(path, sizeof(path), "/dev/%s", name);
        return path;
    }
    #endif
    // Last resort fallback
    snprintf(path, sizeof(path), "/dev/gpiochip?");
    return path;
    #endif
#endif
}