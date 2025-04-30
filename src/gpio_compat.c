#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "gpio_compat.h"

/**
 * This file provides a compatibility layer for libgpiod v1 and v2.
 * It detects the version at compile time and provides a unified API.
 */

#if GPIOD_VERSION_MAJOR >= 2
// libgpiod v2 implementation

struct gpiod_line *gpio_compat_get_line(struct gpiod_chip *chip, unsigned int offset)
{
    return gpiod_chip_get_line(chip, offset);
}

int gpio_compat_request_events(struct gpiod_line *line, const char *consumer, int direction)
{
    struct gpiod_line_request_config config = {
        .consumer = consumer,
        .request_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE,
        .flags = 0,
    };

    if (direction == 0) {
        config.request_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
    } else if (direction == 2) {
        config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
    }

    return gpiod_line_request(line, &config, 0);
}

void gpio_compat_release_line(struct gpiod_line *line)
{
    gpiod_line_release(line);
}

int gpio_compat_get_fd(struct gpiod_line *line)
{
    return gpiod_line_event_get_fd(line);
}

int gpio_compat_read_event(struct gpiod_line *line, struct gpio_compat_event *event)
{
    struct gpiod_line_event gpiod_event;
    int ret = gpiod_line_event_read(line, &gpiod_event);
    if (ret < 0) return ret;

    event->timestamp = gpiod_event.ts.tv_sec * 1000000000ULL + gpiod_event.ts.tv_nsec;
    event->event_type = (gpiod_event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? 1 : 0;
    
    return 0;
}

const char *gpio_compat_get_chip_path(struct gpiod_chip *chip)
{
    return gpiod_chip_name(chip);
}

int gpio_compat_get_value(struct gpiod_line *line, int *success)
{
    *success = 1;
    return gpiod_line_get_value(line);
}

#else
// libgpiod v1 implementation

struct gpiod_line *gpio_compat_get_line(struct gpiod_chip *chip, unsigned int offset)
{
    return gpiod_chip_get_line(chip, offset);
}

int gpio_compat_request_events(struct gpiod_line *line, const char *consumer, int direction)
{
    int event_type;
    
    if (direction == 0) {
        event_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
    } else if (direction == 1) {
        event_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
    } else {
        event_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
    }
    
    return gpiod_line_request_events(line, consumer, event_type, 0);
}

void gpio_compat_release_line(struct gpiod_line *line)
{
    gpiod_line_release(line);
}

int gpio_compat_get_fd(struct gpiod_line *line)
{
    return gpiod_line_event_get_fd(line);
}

int gpio_compat_read_event(struct gpiod_line *line, struct gpio_compat_event *event)
{
    struct gpiod_line_event gpiod_event;
    int ret = gpiod_line_event_read(line, &gpiod_event);
    if (ret < 0) return ret;

    event->timestamp = gpiod_event.ts.tv_sec * 1000000000ULL + gpiod_event.ts.tv_nsec;
    event->event_type = (gpiod_event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? 1 : 0;
    
    return 0;
}

const char *gpio_compat_get_chip_path(struct gpiod_chip *chip)
{
    return gpiod_chip_name(chip);
}

int gpio_compat_get_value(struct gpiod_line *line, int *success)
{
    *success = 1;
    return gpiod_line_get_value(line);
}

#endif /* GPIOD_VERSION_MAJOR */