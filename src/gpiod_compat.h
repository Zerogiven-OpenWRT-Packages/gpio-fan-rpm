//
// gpiod_compat.h - Automatic kernel version detection for libgpiod v1/v2
//
// This file provides compatibility definitions to bridge the gap between
// libgpiod v1 (OpenWRT 23.05/kernel 5.15) and v2 (OpenWRT 24.10/kernel 6.6)
//

#ifndef GPIOD_COMPAT_H
#define GPIOD_COMPAT_H

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// Detect libgpiod version at compile time (from headers)
// Use the same detection logic already in gpio.c
#if defined(GPIOD_VERSION_STR) || defined(GPIOD_API_VERSION)
    // We're building with libgpiod v2 headers
    #define USING_LIBGPIOD_V2 1
    
    #ifdef DEBUG
    #pragma message "Compiling with libgpiod v2 headers"
    #endif
    
    // Make sure v1 event constants are defined for code that might use them
    #ifndef GPIOD_LINE_EVENT_RISING_EDGE
        #define GPIOD_LINE_EVENT_RISING_EDGE GPIOD_EDGE_EVENT_RISING_EDGE
    #endif
    #ifndef GPIOD_LINE_EVENT_FALLING_EDGE
        #define GPIOD_LINE_EVENT_FALLING_EDGE GPIOD_EDGE_EVENT_FALLING_EDGE
    #endif
    
    // Add structure definitions for v1 compatibility
    #ifndef GPIOD_LINE_EVENT_DEFINED
        #define GPIOD_LINE_EVENT_DEFINED
        struct gpiod_line_event {
            struct timespec ts;
            int event_type;
        };
    #endif
    
#else
    // We're building with libgpiod v1 headers (or earlier)
    #define USING_LIBGPIOD_V1 1
    
    #ifdef DEBUG
    #pragma message "Compiling with libgpiod v1 headers"
    #endif
    
    // Define v2 constants for v1 code
    #ifndef GPIOD_LINE_EDGE_RISING
        #define GPIOD_LINE_EDGE_RISING 1
    #endif
    #ifndef GPIOD_LINE_EDGE_FALLING
        #define GPIOD_LINE_EDGE_FALLING 2
    #endif
    #ifndef GPIOD_LINE_EDGE_BOTH
        #define GPIOD_LINE_EDGE_BOTH 3
    #endif
    
    // Add v2 types as dummy types
    #ifndef GPIOD_EDGE_EVENT_BUFFER_DEFINED
        #define GPIOD_EDGE_EVENT_BUFFER_DEFINED
        struct gpiod_edge_event_buffer { void *opaque; };
        struct gpiod_edge_event { struct timespec ts; int event_type; unsigned int line_offset; };
    #endif
    
    // v2 API structure placeholders
    #ifndef GPIOD_LINE_SETTINGS_DEFINED
        #define GPIOD_LINE_SETTINGS_DEFINED
        struct gpiod_line_settings { void *opaque; };
        struct gpiod_line_config { void *opaque; };
        struct gpiod_request_config { void *opaque; };
        struct gpiod_line_request { void *opaque; };
    #endif
    
    // Stubs for v2 functions when compiling with v1
    static inline struct gpiod_line_settings *gpiod_line_settings_new(void) { return NULL; }
    static inline void gpiod_line_settings_free(struct gpiod_line_settings *settings) { (void)settings; }
    static inline int gpiod_line_settings_set_direction(struct gpiod_line_settings *settings, int direction) { (void)settings; (void)direction; return -1; }
    static inline int gpiod_line_settings_set_edge_detection(struct gpiod_line_settings *settings, int edge) { (void)settings; (void)edge; return -1; }
    static inline int gpiod_line_settings_set_bias(struct gpiod_line_settings *settings, int bias) { (void)settings; (void)bias; return -1; }
    static inline struct gpiod_line_config *gpiod_line_config_new(void) { return NULL; }
    static inline void gpiod_line_config_free(struct gpiod_line_config *config) { (void)config; }
    static inline int gpiod_line_config_add_line_settings(struct gpiod_line_config *config, const unsigned int *offsets, unsigned int num_offsets, struct gpiod_line_settings *settings) { (void)config; (void)offsets; (void)num_offsets; (void)settings; return -1; }
    static inline struct gpiod_request_config *gpiod_request_config_new(void) { return NULL; }
    static inline void gpiod_request_config_free(struct gpiod_request_config *config) { (void)config; }
    static inline int gpiod_request_config_set_consumer(struct gpiod_request_config *config, const char *consumer) { (void)config; (void)consumer; return -1; }
    static inline struct gpiod_line_request *gpiod_chip_request_lines(struct gpiod_chip *chip, struct gpiod_request_config *req_cfg, struct gpiod_line_config *line_cfg) { (void)chip; (void)req_cfg; (void)line_cfg; return NULL; }
    static inline void gpiod_line_request_release(struct gpiod_line_request *request) { (void)request; }
    static inline int gpiod_line_request_get_fd(struct gpiod_line_request *request) { (void)request; return -1; }
    static inline int gpiod_line_request_get_value(struct gpiod_line_request *request, unsigned int offset) { (void)request; (void)offset; return -1; }
    static inline struct gpiod_edge_event_buffer *gpiod_edge_event_buffer_new(size_t capacity) { (void)capacity; return NULL; }
    static inline void gpiod_edge_event_buffer_free(struct gpiod_edge_event_buffer *buffer) { (void)buffer; }
    static inline int gpiod_line_request_read_edge_events(struct gpiod_line_request *request, struct gpiod_edge_event_buffer *buffer, size_t max_events) { (void)request; (void)buffer; (void)max_events; return -1; }
    // For v2 compatibility, define functions to get edge event information
    static inline struct gpiod_edge_event *gpiod_edge_event_buffer_get_event(struct gpiod_edge_event_buffer *buffer, unsigned int idx) { (void)buffer; (void)idx; return NULL; }
    static inline struct timespec gpiod_edge_event_get_timestamp(struct gpiod_edge_event *event) { struct timespec ts = {0, 0}; (void)event; return ts; }
    static inline int gpiod_edge_event_get_event_type(struct gpiod_edge_event *event) { (void)event; return -1; }
#endif

// For debugging - print which version we're using
#ifdef DEBUG
    #if defined(USING_LIBGPIOD_V2)
        #pragma message "Using libgpiod v2 API"
    #elif defined(USING_LIBGPIOD_V1)
        #pragma message "Using libgpiod v1 API"
    #endif
#endif

#endif // GPIOD_COMPAT_H