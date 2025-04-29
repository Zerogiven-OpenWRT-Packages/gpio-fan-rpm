#ifndef GPIOD_COMPAT_H
#define GPIOD_COMPAT_H

#include <time.h>
#include <gpiod.h>

// Check if we're using libgpiod v2 API
#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // Define a marker for v2 detection
    #define GPIOD_API_V2

    // For v2 API, we need to define v1 structures and functions if they don't exist
    #ifndef GPIOD_LINE_EVENT_RISING_EDGE
    #define GPIOD_LINE_EVENT_RISING_EDGE 1
    #endif

    #ifndef GPIOD_LINE_EVENT_FALLING_EDGE
    #define GPIOD_LINE_EVENT_FALLING_EDGE 2
    #endif

    // Define the v1 event structure
    #ifndef HAVE_GPIOD_LINE_EVENT_STRUCT
    struct gpiod_line_event {
        struct timespec ts;
        int event_type;
    };
    #define HAVE_GPIOD_LINE_EVENT_STRUCT 1
    #endif

    // Declare v1 API function prototypes
    #ifdef __cplusplus
    extern "C" {
    #endif

    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
    int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
    int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer);
    int gpiod_line_event_get_fd(struct gpiod_line *line);
    void gpiod_line_release(struct gpiod_line *line);
    int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event);
    const char *gpiod_chip_get_path(struct gpiod_chip *chip);

    #ifdef __cplusplus
    }
    #endif

#else
    // We're using libgpiod v1 API
    #define GPIOD_API_V1

    // No compat functions needed for v1, but we might need some forward declarations
    // to prevent implicit function declarations
    #ifndef gpiod_chip_get_path
    const char *gpiod_chip_get_path(struct gpiod_chip *chip);
    #endif
#endif

#endif /* GPIOD_COMPAT_H */