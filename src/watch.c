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
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include "watch.h"
#include "gpio.h"
#include "chip.h"
#include "format.h"
#include <math.h>

// External variable for signal handling
extern volatile int stop;

// Function to monitor keyboard input for 'q' key
static void* keyboard_monitor_thread(void *arg) {
    (void)arg;
    
    // Set terminal to non-blocking mode
    struct termios old_termios, new_termios;
    int old_flags;
    
    // Get current terminal settings
    if (tcgetattr(STDIN_FILENO, &old_termios) != 0) {
        return NULL; // Can't modify terminal, just return
    }
    
    // Save old flags
    old_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    
    // Set non-blocking mode
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0) {
        return NULL; // Can't modify terminal, just return
    }
    
    fcntl(STDIN_FILENO, F_SETFL, old_flags | O_NONBLOCK);
    
    // Monitor for 'q' key
    while (!stop) {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == 'q' || ch == 'Q') {
                stop = 1;
                break;
            }
        }
        usleep(100000); // Sleep 100ms between checks
    }
    
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    fcntl(STDIN_FILENO, F_SETFL, old_flags);
    
    return NULL;
}

int run_watch_mode(int *gpios, size_t ngpio, char *chipname,
                   int duration, int pulses, int debug, output_mode_t mode) {
    // Print watch mode info
    fprintf(stderr, "\nWatch mode started. Press 'q' to quit or Ctrl+C to interrupt.\n\n");
    
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
    
    // Auto-detect chip once for all GPIOs if not specified
    if (!chipname) {
        // Use the first GPIO to detect the chip
        chipname = NULL;
        struct gpiod_chip *test_chip = chip_auto_detect(gpios[0], &chipname);
        if (test_chip) {
            chip_close(test_chip);
        } else {
            fprintf(stderr, "Error: cannot auto-detect GPIO chip\n");
            free(threads);
            free(results);
            free(finished);
            return -1;
        }
    }
    
    // Create keyboard monitor thread
    pthread_t keyboard_thread;
    int keyboard_ret = pthread_create(&keyboard_thread, NULL, keyboard_monitor_thread, NULL);
    if (keyboard_ret) {
        fprintf(stderr, "Warning: cannot create keyboard monitor thread: %s\n", strerror(keyboard_ret));
        fprintf(stderr, "Use Ctrl+C to quit watch mode\n");
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
            
            // Use timed wait to check stop flag periodically
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1; // Wait up to 1 second
            pthread_cond_timedwait(&all_finished, &results_mutex, &ts);
        }
        
        // Only output if we weren't interrupted
        if (!stop) {
            // Output results in order
            if (mode == MODE_JSON && ngpio > 1) {
                // Output as JSON array
                printf("[");
                for (size_t i = 0; i < ngpio; i++) {
                    if (i > 0) printf(",");
                    printf("{\"gpio\":%d,\"rpm\":%d}", gpios[i], (int)round(results[i]));
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
    
    // Wait for all threads to finish
    for (size_t i = 0; i < ngpio; i++) {
        if (threads[i]) {
            pthread_join(threads[i], NULL);
        }
    }
    
    // Wait for keyboard monitor thread
    if (keyboard_ret == 0) {
        pthread_join(keyboard_thread, NULL);
    }
    
    // Cleanup
    pthread_mutex_destroy(&results_mutex);
    pthread_cond_destroy(&all_finished);
    free(threads);
    free(results);
    free(finished);
    
    return 0;
} 