#ifndef GPIOD_COMPAT_H
#define GPIOD_COMPAT_H

#include <time.h>
#include <gpiod.h>

// Define constants if not already defined
#ifndef GPIOD_LINE_EVENT_RISING_EDGE
#define GPIOD_LINE_EVENT_RISING_EDGE 1
#endif

#ifndef GPIOD_LINE_EVENT_FALLING_EDGE
#define GPIOD_LINE_EVENT_FALLING_EDGE 2
#endif

// Define a compatibility event structure that works in both v1 and v2
struct compat_gpiod_line_event {
    struct timespec ts;
    int event_type;
};

#ifdef __cplusplus
extern "C" {
#endif

// Compatibility functions with compat_ prefix to prevent conflicts
struct gpiod_line *compat_gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
int compat_gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
int compat_gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer);
int compat_gpiod_line_event_get_fd(struct gpiod_line *line);
void compat_gpiod_line_release(struct gpiod_line *line);
int compat_gpiod_line_event_read(struct gpiod_line *line, struct compat_gpiod_line_event *event);
const char *compat_gpiod_chip_get_path(struct gpiod_chip *chip);

#ifdef __cplusplus
}
#endif

#endif /* GPIOD_COMPAT_H */