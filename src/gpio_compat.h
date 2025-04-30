#ifndef GPIO_COMPAT_H
#define GPIO_COMPAT_H

#include <time.h>
#include <gpiod.h>

// Detect which API version we're using
#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    #define GPIO_USING_V2_API 1
#else
    #define GPIO_USING_V1_API 1
#endif

// Simple compatibility structure for events
struct gpio_compat_event {
    struct timespec ts;
    int event_type;
};

// Event type definitions
#define GPIO_COMPAT_RISING_EDGE 1
#define GPIO_COMPAT_FALLING_EDGE 2

// Function declarations for our compatibility layer
#ifdef __cplusplus
extern "C" {
#endif

struct gpiod_line *gpio_compat_get_line(struct gpiod_chip *chip, unsigned int offset);
int gpio_compat_request_events(struct gpiod_line *line, const char *consumer, int falling_only);
int gpio_compat_get_fd(struct gpiod_line *line);
void gpio_compat_release_line(struct gpiod_line *line);
int gpio_compat_read_event(struct gpiod_line *line, struct gpio_compat_event *event);
const char *gpio_compat_get_chip_path(struct gpiod_chip *chip);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_COMPAT_H */