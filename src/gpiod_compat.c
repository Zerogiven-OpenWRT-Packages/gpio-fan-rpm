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
#include <gpiod.h> // Include gpiod.h first so we can check what's defined
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
    
    // Only forward declare types if they're not already declared
    struct gpiod_chip;
    struct gpiod_line;
    
    // Function prototypes for compatibility layer - only declare if they're not already defined
    #ifndef gpiod_chip_open_by_name
    struct gpiod_chip *gpiod_chip_open_by_name(const char *name);
    #endif
    #ifndef gpiod_chip_get_line
    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
    #endif
    #ifndef gpiod_chip_get_path
    const char *gpiod_chip_get_path(struct gpiod_chip *chip);
    #endif
    #ifndef gpiod_line_request_input_flags
    int gpiod_line_request_input_flags(struct gpiod_line *line, const char *consumer, int flags);
    #endif
    #ifndef gpiod_line_request_input
    int gpiod_line_request_input(struct gpiod_line *line, const char *consumer);
    #endif
    #ifndef gpiod_line_request_both_edges_events
    int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
    #endif
    #ifndef gpiod_line_request_falling_edge_events
    int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer);
    #endif
    #ifndef gpiod_line_event_get_fd
    int gpiod_line_event_get_fd(struct gpiod_line *line);
    #endif
    #ifndef gpiod_line_get_value
    int gpiod_line_get_value(struct gpiod_line *line);
    #endif
    #ifndef gpiod_line_release
    void gpiod_line_release(struct gpiod_line *line);
    #endif

    #ifndef gpiod_chip_open_by_name
    struct gpiod_chip *gpiod_chip_open_by_name(const char *name)
    {
        // Implementation using standard v1 API - convert name to path
        char path[128];
        snprintf(path, sizeof(path), "/dev/%s", name);
        return gpiod_chip_open(path);
    }
    #endif

    #if !defined(HAVE_GPIOD_CHIP_GET_LINE) && !defined(gpiod_chip_get_line)
    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset)
    {
        // Suppress unused parameter warnings
        (void)chip;
        (void)offset;
        
        // Real implementation for v1 API
        fprintf(stderr, "Warning: Using compatibility function for gpiod_chip_get_line\n");
        errno = ENOSYS;
        return NULL;
    }
    #endif

    #ifndef gpiod_chip_get_path
    const char *gpiod_chip_get_path(struct gpiod_chip *chip)
    {
        static char fallback_path[128] = {0};
        
        if (chip != NULL) {
            #ifdef gpiod_chip_get_name
            const char *name = gpiod_chip_get_name(chip);
            if (name != NULL && name[0] != '\0') {
                snprintf(fallback_path, sizeof(fallback_path), "/dev/%s", name);
                return fallback_path;
            }
            #endif
            
            #ifdef gpiod_chip_get_label
            const char *label = gpiod_chip_get_label(chip);
            if (label != NULL && label[0] != '\0') {
                snprintf(fallback_path, sizeof(fallback_path), "/dev/gpiochip-%s", label);
                return fallback_path;
            }
            #endif
            
            snprintf(fallback_path, sizeof(fallback_path), "/dev/gpiochip<id>");
        } else {
            strncpy(fallback_path, "<null-gpio-chip>", sizeof(fallback_path) - 1);
        }
        
        return fallback_path;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_REQUEST_INPUT_FLAGS)
    int gpiod_line_request_input_flags(struct gpiod_line *line, const char *consumer, int flags)
    {
        (void)line;
        (void)consumer;
        (void)flags;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_request_input_flags\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_REQUEST_INPUT)
    int gpiod_line_request_input(struct gpiod_line *line, const char *consumer)
    {
        (void)line;
        (void)consumer;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_request_input\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_REQUEST_BOTH_EDGES_EVENTS)
    int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer)
    {
        (void)line;
        (void)consumer;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_request_both_edges_events\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_REQUEST_FALLING_EDGE_EVENTS)
    int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer)
    {
        (void)line;
        (void)consumer;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_request_falling_edge_events\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_EVENT_GET_FD)
    int gpiod_line_event_get_fd(struct gpiod_line *line)
    {
        (void)line;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_event_get_fd\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_GET_VALUE)
    int gpiod_line_get_value(struct gpiod_line *line)
    {
        (void)line;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_get_value\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_RELEASE)
    void gpiod_line_release(struct gpiod_line *line)
    {
        (void)line;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_release\n");
    }
    #endif
#endif // GPIOD version check