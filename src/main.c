/**
 * @file main.c
 * @brief Main entry point for gpio-fan-rpm utility
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 * 
 * This module orchestrates the RPM measurement process by parsing arguments,
 * setting up signal handling, and delegating to appropriate measurement modules.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include "gpio.h"
#include "args.h"
#include "watch.h"
#include "measure.h"

// Global variables
volatile int stop = 0;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Signal handler for graceful shutdown
 * 
 * @param sig Signal number
 */
static void signal_handler(int sig) {
    (void)sig;
    stop = 1;
    // Add newline after interrupt for cleaner output
    fprintf(stderr, " ");
}

/**
 * @brief Main function
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return int Exit code
 */
int main(int argc, char **argv) {
    int duration = 2, pulses = 4;
    char *chipname = NULL;
    int debug = 0, watch = 0;
    output_mode_t mode = MODE_DEFAULT;
    int *gpios = NULL;
    size_t ngpio = 0;
    int exit_code = 0;
    
    // Parse command-line arguments
    int parse_result = parse_arguments(argc, argv, &gpios, &ngpio, &chipname, 
                                     &duration, &pulses, &debug, &watch, &mode);
    if (parse_result != 0) {
        free(gpios);
        if (chipname) free(chipname);
        return parse_result > 0 ? 0 : 1; // Help/version return 0, error return 1
    }
    
    // Validate arguments
    if (validate_arguments(gpios, ngpio, duration, pulses, argv[0]) != 0) {
        free(gpios);
        if (chipname) free(chipname);
        return 1;
    }
    
    // Set up signal handlers for graceful shutdown
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "Warning: Failed to set up SIGINT handler\n");
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        fprintf(stderr, "Warning: Failed to set up SIGTERM handler\n");
    }
    
    // Run appropriate measurement mode
    int measurement_result;
    if (watch) {
        // Continuous monitoring mode
        measurement_result = run_watch_mode(gpios, ngpio, chipname, duration, 
                                          pulses, debug, mode);
    } else {
        // Single measurement mode
        measurement_result = run_single_measurement(gpios, ngpio, chipname, duration, 
                                                  pulses, debug, mode);
    }
    
    // Cleanup
    free(gpios);
    if (chipname) free(chipname);
    
    // Set appropriate exit code
    if (measurement_result != 0) {
        exit_code = 1;
        if (!debug) {
            fprintf(stderr, "Error: Measurement failed. Use --debug for details.\n");
        }
    }
    
    return exit_code;
} 