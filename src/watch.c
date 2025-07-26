/**
 * @file watch.c
 * @brief Watch mode functionality for continuous RPM monitoring
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 * 
 * This module handles continuous monitoring mode with parallel
 * measurement and ordered output for multiple GPIO pins.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "watch.h"
#include "gpio.h"
#include "format.h"

// External variable for signal handling
extern volatile int stop;

int run_watch_mode(int *gpios, size_t ngpio, char *chipname,
                   int duration, int pulses, int debug, output_mode_t mode) {
    // Create persistent threads for watch mode
    pthread_t *threads = calloc(ngpio, sizeof(*threads));
    if (!threads) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return -1;
    }
    
    // Allocate result storage
    double *results = calloc(ngpio, sizeof(*results));
    int *finished = calloc(ngpio, sizeof(*finished));
    if (!results || !finished) {
        fprintf(stderr, "Error: memory allocation failed\n");
        free(threads);
        free(results);
        free(finished);
        return -1;
    }
    
    // Synchronization primitives
    pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t all_finished = PTHREAD_COND_INITIALIZER;
    
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
        a->watch = 1; // Always true for watch mode
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
    
    // Main loop for watch mode
    while (!stop) {
        // Wait for all threads to complete one measurement round
        pthread_mutex_lock(&results_mutex);
        while (!stop) {
            int all_done = 1;
            for (size_t i = 0; i < ngpio; i++) {
                if (!finished[i]) {
                    all_done = 0;
                    break;
                }
            }
            if (all_done) break;
            pthread_cond_wait(&all_finished, &results_mutex);
        }
        
        // Only output if we weren't interrupted
        if (!stop) {
            // Output results in order
            if (mode == MODE_JSON && ngpio > 1) {
                // Output as JSON array
                printf("[");
                for (size_t i = 0; i < ngpio; i++) {
                    if (i > 0) printf(",");
                    printf("{\"gpio\":%d,\"rpm\":%.0f}", gpios[i], results[i]);
                }
                printf("]\n");
            } else {
                // Output individual results in order
                for (size_t i = 0; i < ngpio; i++) {
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
            
            // Reset finished flags for next round
            memset(finished, 0, ngpio * sizeof(*finished));
        }
        
        pthread_mutex_unlock(&results_mutex);
    }
    
    // Cleanup
    pthread_mutex_destroy(&results_mutex);
    pthread_cond_destroy(&all_finished);
    free(threads);
    free(results);
    free(finished);
    
    return 0;
} 