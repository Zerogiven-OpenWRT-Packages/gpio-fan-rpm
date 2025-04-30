#ifndef GPIO_COMPAT_H
#define GPIO_COMPAT_H

#include <gpiod.h>

// Event structure for compatibility layer
struct gpio_compat_event {
    uint64_t timestamp;  // Timestamp in nanoseconds
    int event_type;      // 1 = rising edge, 0 = falling edge
};

// Get line from chip and offset
struct gpiod_line *gpio_compat_get_line(struct gpiod_chip *chip, unsigned int offset);

// Request events on a line (for edge detection)
// direction: 0 = rising, 1 = falling, 2 = both
int gpio_compat_request_events(struct gpiod_line *line, const char *consumer, int direction);

// Release a line
void gpio_compat_release_line(struct gpiod_line *line);

// Get file descriptor for a line
int gpio_compat_get_fd(struct gpiod_line *line);

// Read an event from a line
int gpio_compat_read_event(struct gpiod_line *line, struct gpio_compat_event *event);

// Get chip path
const char *gpio_compat_get_chip_path(struct gpiod_chip *chip);

// Get line value
int gpio_compat_get_value(struct gpiod_line *line, int *success);

#endif /* GPIO_COMPAT_H */