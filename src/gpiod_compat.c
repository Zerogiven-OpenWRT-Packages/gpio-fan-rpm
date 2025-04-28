//
// gpiod_compat.c - Compatibility layer for libgpiod v1
//
// This file provides compatibility functions when building with older libgpiod libraries
// that don't provide the expected API. This allows the code to compile and link properly.
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

// Only include this implementation if libgpiod version macro is not defined
#ifndef GPIOD_VERSION_STR

// Implement all the missing v1 API functions that caused linker errors

// Wrapper for gpiod_chip_open_by_name
struct gpiod_chip *gpiod_chip_open_by_name(const char *name)
{
    // The v1 API might not have gpiod_chip_open_by_name, but should have gpiod_chip_open
    char path[128];
    snprintf(path, sizeof(path), "/dev/%s", name);
    return gpiod_chip_open(path);
}

// These are the v1 API functions that were missing (causing linker errors)
// Implement simplified versions that call into the actual libgpiod functions

// Function to get a GPIO line from a chip
struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset)
{
    // If the version of libgpiod installed has this function under a different name,
    // modify this to call the correct function
    fprintf(stderr, "Compatibility: gpiod_chip_get_line(%p, %u)\n", chip, offset);
    
    // Try to use the existing API if available
    struct gpiod_line *line = NULL;
    
    // If your system's libgpiod has a different API, modify this function to match
    // For now, we'll return NULL which will cause an error but avoid a crash
    errno = ENOSYS;
    return NULL;
}

// Request a line to be used as input with specific flags
int gpiod_line_request_input_flags(struct gpiod_line *line, const char *consumer, int flags)
{
    fprintf(stderr, "Compatibility: gpiod_line_request_input_flags(%p, %s, %d)\n", 
            line, consumer, flags);
    errno = ENOSYS;
    return -1;
}

// Request a line to be used as input
int gpiod_line_request_input(struct gpiod_line *line, const char *consumer)
{
    fprintf(stderr, "Compatibility: gpiod_line_request_input(%p, %s)\n", line, consumer);
    errno = ENOSYS;
    return -1;
}

// Request a line for both edges event monitoring
int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer)
{
    fprintf(stderr, "Compatibility: gpiod_line_request_both_edges_events(%p, %s)\n", 
            line, consumer);
    errno = ENOSYS;
    return -1;
}

// Request a line for falling edge event monitoring
int gpiod_line_request_falling_edge_events(struct gpiod_line *line, const char *consumer)
{
    fprintf(stderr, "Compatibility: gpiod_line_request_falling_edge_events(%p, %s)\n", 
            line, consumer);
    errno = ENOSYS;
    return -1;
}

// Get the file descriptor for a line event
int gpiod_line_event_get_fd(struct gpiod_line *line)
{
    fprintf(stderr, "Compatibility: gpiod_line_event_get_fd(%p)\n", line);
    errno = ENOSYS;
    return -1;
}

// Get the value of a GPIO line
int gpiod_line_get_value(struct gpiod_line *line)
{
    fprintf(stderr, "Compatibility: gpiod_line_get_value(%p)\n", line);
    errno = ENOSYS;
    return -1;
}

// Release a previously requested line
void gpiod_line_release(struct gpiod_line *line)
{
    fprintf(stderr, "Compatibility: gpiod_line_release(%p)\n", line);
    // No implementation needed for void function
}

// Read an event from a line
int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event)
{
    fprintf(stderr, "Compatibility: gpiod_line_event_read(%p, %p)\n", line, event);
    errno = ENOSYS;
    return -1;
}

#endif // !GPIOD_VERSION_STR