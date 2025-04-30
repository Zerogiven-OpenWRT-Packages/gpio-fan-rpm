#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <gpiod.h>
#include "gpio_compat.h"

// This file is specifically for v1 API (OpenWRT 23.05)

// Get a line handle from a chip
struct gpiod_line *gpio_compat_get_line(struct gpiod_chip *chip, unsigned int offset)
{
    return gpiod_chip_get_line(chip, offset);
}

// Request edge events on a line
int gpio_compat_request_events(struct gpiod_line *line, const char *consumer, int falling_only)
{
    if (falling_only) {
        return gpiod_line_request_falling_edge_events(line, consumer);
    } else {
        return gpiod_line_request_both_edges_events(line, consumer);
    }
}

// Get a file descriptor for polling
int gpio_compat_get_fd(struct gpiod_line *line)
{
    return gpiod_line_event_get_fd(line);
}

// Release resources associated with a line
void gpio_compat_release_line(struct gpiod_line *line)
{
    gpiod_line_release(line);
}

// Read an event from a line
int gpio_compat_read_event(struct gpiod_line *line, struct gpio_compat_event *event)
{
    struct gpiod_line_event v1_event;
    int ret = gpiod_line_event_read(line, &v1_event);
    
    if (ret < 0) return ret;
    
    // Copy to our common structure
    event->ts = v1_event.ts;
    
    // Convert event type to our constants
    if (v1_event.event_type == 1) { // GPIOD_LINE_EVENT_RISING_EDGE in v1
        event->event_type = GPIO_COMPAT_RISING_EDGE;
    } else {
        event->event_type = GPIO_COMPAT_FALLING_EDGE;
    }
    
    return 0;
}

// Get the path for a chip
const char *gpio_compat_get_chip_path(struct gpiod_chip *chip)
{
    static char path[128];
    const char *name = gpiod_chip_name(chip);
    if (name) {
        snprintf(path, sizeof(path), "/dev/%s", name);
    } else {
        snprintf(path, sizeof(path), "/dev/gpiochip?");
    }
    return path;
}