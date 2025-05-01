//
// gpiod_compat.h - Compatibility header for libgpiod v1/v2
//
// This file provides compatibility definitions to bridge the gap between
// libgpiod v1 and v2 APIs
//

#ifndef GPIOD_COMPAT_H
#define GPIOD_COMPAT_H

#include <gpiod.h>
#include <time.h>

// Detect libgpiod version at compile time
#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // We have libgpiod v2 headers
    #define USING_LIBGPIOD_V2 1
    
    // Define v1 event structure for compatibility - must be defined before any other includes
    #ifndef GPIOD_LINE_EVENT_DEFINED
        #define GPIOD_LINE_EVENT_DEFINED
        struct gpiod_line_event {
            struct timespec ts;
            int event_type;
        };
    #endif
    
    // Define v1 constants based on v2 constants
    #ifndef GPIOD_LINE_EVENT_RISING_EDGE
        #define GPIOD_LINE_EVENT_RISING_EDGE 1
    #endif
    #ifndef GPIOD_LINE_EVENT_FALLING_EDGE
        #define GPIOD_LINE_EVENT_FALLING_EDGE 2
    #endif
    
    // Forward declare v1 API functions (implemented in gpiod_compat.c)
    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
    int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
    int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer);
    int gpiod_line_event_get_fd(struct gpiod_line *line);
    int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event);
    void gpiod_line_release(struct gpiod_line *line);
    int gpiod_line_get_value(struct gpiod_line *line);
    
#else
    // We have libgpiod v1 headers
    #define USING_LIBGPIOD_V1 1
    
    // Make sure v1 constants are defined
    #ifndef GPIOD_LINE_EVENT_RISING_EDGE
        #define GPIOD_LINE_EVENT_RISING_EDGE 1
    #endif
    #ifndef GPIOD_LINE_EVENT_FALLING_EDGE
        #define GPIOD_LINE_EVENT_FALLING_EDGE 2
    #endif
    
    // Define v2 constants based on v1 constants for code compatibility
    #ifndef GPIOD_LINE_EDGE_RISING
        #define GPIOD_LINE_EDGE_RISING GPIOD_LINE_EVENT_RISING_EDGE
    #endif
    #ifndef GPIOD_LINE_EDGE_FALLING
        #define GPIOD_LINE_EDGE_FALLING GPIOD_LINE_EVENT_FALLING_EDGE
    #endif
    #ifndef GPIOD_LINE_EDGE_BOTH
        #define GPIOD_LINE_EDGE_BOTH 3
    #endif
    
    // Bias and direction macros for v1 compatibility
    #ifndef GPIOD_LINE_DIRECTION_INPUT
        #define GPIOD_LINE_DIRECTION_INPUT 1
    #endif
    #ifndef GPIOD_LINE_BIAS_PULL_UP
        #define GPIOD_LINE_BIAS_PULL_UP 2
    #endif
#endif

// Debug helper to show which API we're using
#ifdef DEBUG
    #if defined(USING_LIBGPIOD_V2)
        #pragma message "Building with libgpiod v2 API and v1 compatibility layer"
    #elif defined(USING_LIBGPIOD_V1)
        #pragma message "Building with libgpiod v1 API"
    #endif
#endif

#endif // GPIOD_COMPAT_H