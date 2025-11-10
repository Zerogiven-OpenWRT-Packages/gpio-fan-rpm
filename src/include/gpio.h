/**
 * @file gpio.h
 * @brief GPIO interface for fan RPM measurement
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 *
 * This module provides a unified interface for GPIO operations across
 * different libgpiod versions (v1 and v2) used in OpenWRT 23.05 and 24.10.
 */

#ifndef GPIO_H
#define GPIO_H

#include <gpiod.h>
#include <pthread.h>
#include <stdint.h>
#include "format.h"  // For output_mode_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Thread arguments structure
 */
typedef struct {
    int gpio;                    /**< GPIO number to measure */
    char *chipname;              /**< GPIO chip name (NULL for auto-detect) */
    int duration;                /**< Total measurement duration in seconds */
    int pulses;                  /**< Pulses per revolution */
    int debug;                   /**< Enable debug output */
    int watch;                   /**< Continuous monitoring mode */
    output_mode_t mode;          /**< Output format mode */
    size_t thread_index;         /**< Thread index for ordering */
    size_t total_threads;        /**< Total number of threads */
    double *results;             /**< Array to store measurement results */
    int *finished;               /**< Array to track which threads finished */
    pthread_mutex_t *results_mutex; /**< Mutex for results array */
    pthread_cond_t *all_finished;   /**< Condition variable for all threads finished */
} thread_args_t;

/**
 * @brief GPIO context structure for version compatibility
 */
typedef struct gpio_context {
#ifdef LIBGPIOD_V2
    struct gpiod_chip *chip;
    struct gpiod_line_request *request;
    int event_fd;
#else
    struct gpiod_chip *chip;
    struct gpiod_line *line;
#endif
    int gpio;
    char *chipname;
} gpio_context_t;

// Global variables (extern declarations)
extern volatile int stop;
extern pthread_mutex_t print_mutex;



/**
 * @brief Initialize GPIO context for the specified GPIO
 * 
 * @param gpio GPIO number to measure
 * @param chipname GPIO chip name (NULL for auto-detect)
 * @return gpio_context_t* Pointer to initialized context, NULL on error
 * 
 * @note The returned context must be freed with gpio_cleanup()
 */
gpio_context_t* gpio_init(int gpio, const char *chipname);

/**
 * @brief Clean up GPIO context
 * 
 * @param ctx GPIO context to clean up
 * 
 * @note This function is safe to call with NULL pointer
 */
void gpio_cleanup(gpio_context_t *ctx);

/**
 * @brief Request edge events on GPIO line
 * 
 * @param ctx GPIO context
 * @param consumer Consumer name for the request
 * @return int 0 on success, -1 on error
 */
int gpio_request_events(gpio_context_t *ctx, const char *consumer);

/**
 * @brief Wait for edge event with timeout
 * 
 * @param ctx GPIO context
 * @param timeout_ns Timeout in nanoseconds
 * @return int 0 on timeout, 1 on event, -1 on error
 */
int gpio_wait_event(gpio_context_t *ctx, int64_t timeout_ns);

/**
 * @brief Read edge event
 * 
 * @param ctx GPIO context
 * @return int 0 on success, -1 on error
 */
int gpio_read_event(gpio_context_t *ctx);

/**
 * @brief Measure RPM on a GPIO line
 *
 * @param ctx GPIO context
 * @param pulses_per_rev Pulses per revolution
 * @param duration Total measurement duration in seconds
 * @param debug Enable debug output
 * @return double RPM value, -1.0 if interrupted, 0.0 if no pulses detected
 */
double gpio_measure_rpm(gpio_context_t *ctx, int pulses_per_rev, int duration, int debug);

/**
 * @brief Thread function for GPIO measurement
 * 
 * @param arg Thread arguments (will be freed by this function)
 * @return void* NULL
 */
void* gpio_thread_fn(void *arg);

#ifdef __cplusplus
}
#endif

#endif // GPIO_H 