/**
 * @file measure.c
 * @brief Single measurement functionality for RPM monitoring
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 * 
 * This module handles single measurement mode with parallel
 * measurement and ordered output for multiple GPIO pins.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "measure.h"
#include "gpio.h"
#include "format.h"
#include "chip.h"
#include <math.h>

int run_single_measurement(int *gpios, size_t ngpio, char *chipname,
                          int duration, int pulses, int debug, output_mode_t mode) {
    // Debug timing
    if (debug) {
        fprintf(stderr, "DEBUG: Starting measurement for %zu GPIOs\n", ngpio);
    }
    // Single measurement mode - simpler approach
    double *results = calloc(ngpio, sizeof(*results));
    int *finished = calloc(ngpio, sizeof(*finished));
    if (!results || !finished) {
        fprintf(stderr, "Error: memory allocation failed\n");
        free(results);
        free(finished);
        return -1;
    }
    
    // Synchronization primitives
    pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t all_finished = PTHREAD_COND_INITIALIZER;
    
    // Reset for measurement
    memset(finished, 0, ngpio * sizeof(*finished));
    memset(results, 0, ngpio * sizeof(*results));
    
    // Auto-detect chip once for all GPIOs if not specified
    if (!chipname) {
        // Use the first GPIO to detect the chip
        chipname = NULL;
        struct gpiod_chip *test_chip = chip_auto_detect(gpios[0], &chipname);
        if (test_chip) {
            chip_close(test_chip);
        } else {
            fprintf(stderr, "Error: cannot auto-detect GPIO chip\n");
            pthread_mutex_destroy(&results_mutex);
            pthread_cond_destroy(&all_finished);
            free(results);
            free(finished);
            return -1;
        }
    }

    pthread_t *threads = calloc(ngpio, sizeof(*threads));
    if (!threads) {
        fprintf(stderr, "Error: memory allocation failed\n");
        pthread_mutex_destroy(&results_mutex);
        pthread_cond_destroy(&all_finished);
        free(results);
        free(finished);
        return -1;
    }
    
    // Create threads for each GPIO
    for (size_t i = 0; i < ngpio; i++) {
        thread_args_t *a = calloc(1, sizeof(*a));
        if (!a) {
            fprintf(stderr, "Error: memory allocation failed\n");
            continue;
        }
        
        a->gpio = gpios[i];
        a->chipname = chipname;
        a->duration = duration;
        a->pulses = pulses;
        a->debug = debug;
        a->watch = 0; // Always false for single measurement
        a->mode = mode;
        a->thread_index = i;
        a->total_threads = ngpio;
        a->results = results;
        a->finished = finished;
        a->results_mutex = &results_mutex;
        a->all_finished = &all_finished;
        
        int ret = pthread_create(&threads[i], NULL, gpio_thread_fn, a);
        if (ret) {
            fprintf(stderr, "Error: cannot create thread for GPIO %d: %s\n", 
                    a->gpio, strerror(ret));
            free(a);
            threads[i] = 0;
        }
    }
    
    // Wait for all threads to finish
    for (size_t i = 0; i < ngpio; i++) {
        if (threads[i]) {
            pthread_join(threads[i], NULL);
        }
    }
    
    free(threads);
    
    // Output results in order
    if (mode == MODE_JSON && ngpio > 1) {
        // Output as JSON array
        printf("[");
        int first = 1;
        for (size_t i = 0; i < ngpio; i++) {
            // Skip interrupted measurements (negative values indicate interruption)
            if (results[i] < 0.0) {
                continue;
            }
            if (!first) printf(",");
            printf("{\"gpio\":%d,\"rpm\":%d}", gpios[i], (int)round(results[i]));
            first = 0;
        }
        printf("]\n");
    } else {
            // Output individual results in order
    for (size_t i = 0; i < ngpio; i++) {
        // Skip interrupted measurements (negative values indicate interruption)
        if (results[i] < 0.0) {
            continue;
        }
        
        char *output = NULL;
        switch (mode) {
        case MODE_NUMERIC:
            output = format_numeric(results[i]);
            break;
        case MODE_JSON:
            output = format_json(gpios[i], results[i]);
            break;
        case MODE_COLLECTD:
            output = format_collectd(gpios[i], results[i], duration);
            break;
        default:
            output = format_human_readable(gpios[i], results[i]);
            break;
        }
        
        if (output) {
            printf("%s", output);
            free(output);
        }
    }
    }
    fflush(stdout);
    
    // Cleanup
    pthread_mutex_destroy(&results_mutex);
    pthread_cond_destroy(&all_finished);
    free(results);
    free(finished);
    
    return 0;
} 