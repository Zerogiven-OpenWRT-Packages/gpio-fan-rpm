/**
 * @file watch.h
 * @brief Watch mode functionality for continuous RPM monitoring
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 * 
 * This module handles continuous monitoring mode with parallel
 * measurement and ordered output for multiple GPIO pins.
 */

#ifndef WATCH_H
#define WATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"

/**
 * @brief Run continuous monitoring mode for multiple GPIO pins
 * 
 * @param gpios Array of GPIO numbers
 * @param ngpio Number of GPIOs
 * @param chipname GPIO chip name
 * @param duration Measurement duration
 * @param pulses Pulses per revolution
 * @param debug Debug flag
 * @param mode Output mode
 * @return int 0 on success, -1 on error
 */
int run_watch_mode(int *gpios, size_t ngpio, char *chipname,
                   int duration, int pulses, int debug, output_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_H */ 