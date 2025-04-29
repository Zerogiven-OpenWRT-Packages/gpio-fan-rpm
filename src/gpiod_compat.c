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
    
    // Only forward declare types if they're not already declared
    struct gpiod_chip;
    struct gpiod_line;
    
    // Only define the event structure if not already defined in system headers
    #if !defined(HAVE_GPIOD_LINE_EVENT_STRUCT) && !defined(__GPIOD_STRUCT_LINE_EVENT)
    struct gpiod_line_event {
        struct timespec ts;
        int event_type;
    };
    #define __GPIOD_STRUCT_LINE_EVENT 1  // Mark as defined for internal use
    #endif
    
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
    #ifndef gpiod_line_event_read
    int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event);
    #endif
    
    // These are the v1 API functions we need but might be missing or have different names
    // in the target environment's libgpiod. Only define the ones that 
    // couldn't be resolved in the other source files.

    #ifndef gpiod_chip_open_by_name
    struct gpiod_chip *gpiod_chip_open_by_name(const char *name)
    {
        // Implementation using standard v1 API - convert name to path
        char path[128];
        snprintf(path, sizeof(path), "/dev/%s", name);
        return gpiod_chip_open(path);
    }
    #endif

    // Check for gpiod_chip_get_line
    #if !defined(HAVE_GPIOD_CHIP_GET_LINE) && !defined(gpiod_chip_get_line)
    struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset)
    {
        // Suppress unused parameter warnings
        (void)chip;
        (void)offset;
        
        // Real implementation for v1 API
        // This is actually provided by the libgpiod v1, so this shouldn't be needed
        // But in case it's missing or renamed, we provide a fallback
        fprintf(stderr, "Warning: Using compatibility function for gpiod_chip_get_line\n");
        errno = ENOSYS;
        return NULL;
    }
    #endif

    // Check for gpiod_chip_get_path
    #if !defined(HAVE_GPIOD_CHIP_GET_PATH) && !defined(gpiod_chip_get_path)
    const char *gpiod_chip_get_path(struct gpiod_chip *chip)
    {
        // This is a compatibility function that should return the
        // full device path (e.g., "/dev/gpiochip0") of the GPIO chip.
        // In libgpiod v1, this information is typically available internally.
        
        static char fallback_path[128] = {0};
        
        if (chip != NULL) {
            // In libgpiod v1, there's no direct function to retrieve the path
            // We'll attempt to use the chip name or number as a fallback
            
            // Try to use gpiod_chip_get_name if it exists
            #ifdef gpiod_chip_get_name
            const char *name = gpiod_chip_get_name(chip);
            if (name != NULL && name[0] != '\0') {
                snprintf(fallback_path, sizeof(fallback_path), "/dev/%s", name);
                return fallback_path;
            }
            #endif
            
            // Try to use gpiod_chip_get_label if it exists
            #ifdef gpiod_chip_get_label
            const char *label = gpiod_chip_get_label(chip);
            if (label != NULL && label[0] != '\0') {
                snprintf(fallback_path, sizeof(fallback_path), "/dev/gpiochip-%s", label);
                return fallback_path;
            }
            #endif
            
            // Fallback - assume the chip is at least valid
            // We don't actually have a reliable way to get the path in v1 API without additional functions
            snprintf(fallback_path, sizeof(fallback_path), "/dev/gpiochip<id>");
        } else {
            // Null chip pointer
            strncpy(fallback_path, "<null-gpio-chip>", sizeof(fallback_path) - 1);
        }
        
        return fallback_path;
    }
    #endif

    // Only define the missing v1 functions you need
    // Add more as needed based on linker errors
    #if !defined(HAVE_GPIOD_LINE_REQUEST_INPUT_FLAGS)
    int gpiod_line_request_input_flags(struct gpiod_line *line, const char *consumer, int flags)
    {
        // Suppress unused parameter warnings
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
        // Suppress unused parameter warnings
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
        // Suppress unused parameter warnings
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
        // Suppress unused parameter warnings
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
        // Suppress unused parameter warnings
        (void)line;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_event_get_fd\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_GET_VALUE)
    int gpiod_line_get_value(struct gpiod_line *line)
    {
        // Suppress unused parameter warnings
        (void)line;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_get_value\n");
        errno = ENOSYS;
        return -1;
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_RELEASE)
    void gpiod_line_release(struct gpiod_line *line)
    {
        // Suppress unused parameter warnings
        (void)line;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_release\n");
        // No implementation needed
    }
    #endif

    #if !defined(HAVE_GPIOD_LINE_EVENT_READ)
    int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event)
    {
        // Suppress unused parameter warnings
        (void)line;
        (void)event;
        
        fprintf(stderr, "Warning: Using compatibility function for gpiod_line_event_read\n");
        errno = ENOSYS;
        return -1;
    }
    #endif
#endif // GPIOD version check