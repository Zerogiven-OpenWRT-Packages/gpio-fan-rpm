//
// gpiod_compat.c - Compatibility layer for libgpiod v1/v2
//
// This file provides compatibility functions to bridge the gap between
// libgpiod v1 (OpenWRT 23.05) and v2 (OpenWRT 24.10)
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <gpiod.h>
#include "gpio-fan-rpm.h"

// The approach: we need to detect if we're in an environment with:
// 1. libgpiod v1 API (need to detect v1 function names)
// 2. libgpiod v2 API (needs v1 compatibility functions)
// 3. Mixed/unknown (be cautious, provide compatibility but with warnings)

// First check: Does this header define the v2 version macro?
#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // We're building with a modern libgpiod v2 that defines version macros.
    // No need for compatibility layer.
    #define USING_LIBGPIOD_V2 1
    
    // Empty placeholder to make this file compile without errors
    static void gpiod_compat_not_needed(void) {
        /* This function intentionally left empty */
    }
#else
    // This appears to be a libgpiod v1 environment or an unknown version.
    // Provide compatibility functions conditionally.
    #define USING_LIBGPIOD_V1 1
    
    // Define the structure for gpiod_line_event first so it's available to all functions
    #if !defined(HAVE_STRUCT_GPIOD_LINE_EVENT)
    struct gpiod_line_event {
        struct timespec ts;
        int event_type;
    };
    #endif
    
    // Function prototypes for compatibility layer
    struct gpiod_chip *gpiod_chip_open_by_name(const char *name);
    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
    int gpiod_line_request_input_flags(struct gpiod_line *line, const char *consumer, int flags);
    int gpiod_line_request_input(struct gpiod_line *line, const char *consumer);
    int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
    int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer);
    int gpiod_line_event_get_fd(struct gpiod_line *line);
    int gpiod_line_get_value(struct gpiod_line *line);
    void gpiod_line_release(struct gpiod_line *line);
    int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event);
    
    // These are the v1 API functions we need but might be missing or have different names
    // in the target environment's libgpiod. Only define the ones that 
    // couldn't be resolved in the other source files.

    struct gpiod_chip *gpiod_chip_open_by_name(const char *name)
    {
        // Implementation using standard v1 API - convert name to path
        char path[128];
        snprintf(path, sizeof(path), "/dev/%s", name);
        return gpiod_chip_open(path);
    }

    // Check for gpiod_chip_get_line
    #if !defined(HAVE_GPIOD_CHIP_GET_LINE)
    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset)
    {
        // Real implementation for v1 API
        // This is actually provided by the libgpiod v1, so this shouldn't be needed
        // But in case it's missing or renamed, we provide a fallback
        fprintf(stderr, "Warning: Using compatibility function for gpiod_chip_get_line\n");
        errno = ENOSYS;
        return NULL;
    }
    #endif

    // Only define the missing v1 functions you need
    // Add more as needed based on linker errors
    #if !defined(HAVE_GPIOD_LINE_REQUEST_INPUT_FLAGS)
    int gpiod_line_request_input_flags(struct gpiod_line *line, const char *consumer, int flags)
    {
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_request_input_flags\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_REQUEST_INPUT)
    int gpiod_line_request_input(struct gpiod_line *line, const char *consumer)
    {
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_request_input\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_REQUEST_BOTH_EDGES_EVENTS)
    int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer)
    {
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_request_both_edges_events\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_REQUEST_FALLING_EDGE_EVENTS)
    int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer)
    {
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_request_falling_edge_events\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_EVENT_GET_FD)
    int gpiod_line_event_get_fd(struct gpiod_line *line)
    {
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_event_get_fd\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_GET_VALUE)
    int gpiod_line_get_value(struct gpiod_line *line)
    {
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_get_value\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_RELEASE)
    void gpiod_line_release(struct gpiod_line *line)
    {
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_release\n");
        // No implementation needed
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_EVENT_READ)
    int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event)
    {
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_event_read\n");
        errno = ENOSYS;
        return -1;
    }
    #endif
#endif // GPIOD version check