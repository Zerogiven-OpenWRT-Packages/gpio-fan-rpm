/**
 * @file line.h
 * @brief GPIO line operations
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 * 
 * This module handles GPIO line operations with compatibility for both
 * libgpiod v1 and v2.
 */

#ifndef LINE_H
#define LINE_H

#include <gpiod.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Line request context for version compatibility
 */
typedef struct line_request {
#ifdef LIBGPIOD_V2
    struct gpiod_line_request *request;
    int event_fd;
#else
    struct gpiod_line *line;
#endif
    int gpio;
} line_request_t;

/**
 * @brief Request line for edge events
 * 
 * @param chip GPIO chip
 * @param gpio GPIO line number
 * @param consumer Consumer name
 * @return line_request_t* Line request context or NULL on error
 */
line_request_t* line_request_events(struct gpiod_chip *chip, int gpio, const char *consumer);

/**
 * @brief Release line request
 * 
 * @param req Line request context
 */
void line_release(line_request_t *req);

/**
 * @brief Wait for edge event
 * 
 * @param req Line request context
 * @param timeout_ns Timeout in nanoseconds
 * @return int 0 on timeout, 1 on event, -1 on error
 */
int line_wait_event(line_request_t *req, int64_t timeout_ns);

/**
 * @brief Read edge event
 * 
 * @param req Line request context
 * @return int 0 on success, -1 on error
 */
int line_read_event(line_request_t *req);

#ifdef __cplusplus
}
#endif

#endif // LINE_H 