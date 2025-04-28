// gpio.c
// GPIO setup and value reading using libgpiod v1/v2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // Include errno for error reporting
#include <unistd.h>
#include <gpiod.h>
#include "gpio-fan-rpm.h"

// Detect libgpiod version
#if defined(GPIOD_VERSION_STR)
    // libgpiod v2
    #define LIBGPIOD_V2
#else
    // libgpiod v1
    #define LIBGPIOD_V1
    // Define constants for v1 that might be missing
    #ifndef GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP
        #define GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP (1 << 2)
    #endif
#endif

// Function prototypes for libgpiod v1 API
#ifdef LIBGPIOD_V1
struct gpiod_chip *gpiod_chip_open_by_name(const char *name);
void gpiod_chip_close(struct gpiod_chip *chip);
struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
int gpiod_line_request_input(struct gpiod_line *line, const char *consumer);
int gpiod_line_request_input_flags(struct gpiod_line *line, const char *consumer, int flags);
int gpiod_line_get_value(struct gpiod_line *line);
void gpiod_line_release(struct gpiod_line *line);
#endif

// Open GPIO chip
struct gpiod_chip *gpio_open_chip(const char *chip_name)
{
    struct gpiod_chip *chip = gpiod_chip_open_by_name(chip_name);
    if (!chip)
    {
        fprintf(stderr, "Error: Unable to open GPIO chip '%s'\n", chip_name);
    }
    return chip;
}

// Close GPIO chip
void gpio_close_chip(struct gpiod_chip *chip)
{
    if (chip)
        gpiod_chip_close(chip);
}

// Get GPIO value (single read)
int gpio_get_value(struct gpiod_chip *chip, gpio_info_t *info, int *success)
{
    if (!chip)
    {
        if (success)
            *success = 0;
        return -1;
    }

#ifdef LIBGPIOD_V2
    // libgpiod v2 API
    struct gpiod_line_request_config config = {
        .consumer = "gpio-fan-rpm-read",
        .request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
        .flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP,
    };
    
    struct gpiod_line_request *request = NULL;
    unsigned int offset = info->gpio_rel;
    int ret = -1;
    
    request = gpiod_chip_request_lines(chip, &config, &offset, 1);
    if (!request) {
        // Try without pull-up if it failed
        config.flags = 0;
        request = gpiod_chip_request_lines(chip, &config, &offset, 1);
        if (!request) {
            if (success) *success = 0;
            return -1;
        }
    }
    
    // Read the value
    ret = gpiod_line_request_get_value(request, 0);
    
    // Release the line
    gpiod_line_request_release(request);
    
    if (success) *success = 1;
    return ret;
#else
    // libgpiod v1 API - fallback
    #warning "Compiling gpio_get_value with libgpiod v1 API fallback. GPIO bias settings might not be applied."
    struct gpiod_line *line = gpiod_chip_get_line(chip, info->gpio_rel);
    if (!line)
    {
        fprintf(stderr, "Error: Unable to get GPIO line %d from chip\n", info->gpio_rel);
        if (success)
            *success = 0;
        return -1;
    }

    // Try to request with pull-up bias (might fail on older systems)
    if (gpiod_line_request_input_flags(line, "gpio-fan-rpm-read", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        // Fall back to no pull-up if not supported
        if (gpiod_line_request_input(line, "gpio-fan-rpm-read") < 0) {
            fprintf(stderr, "Error: Unable to request GPIO line %d for input\n", info->gpio_rel);
            gpiod_line_release(line);
            if (success)
                *success = 0;
            return -1;
        }
    }

    int val_int = gpiod_line_get_value(line);
    if (val_int < 0)
    {
        fprintf(stderr, "Error: Unable to read value from GPIO line %d\n", info->gpio_rel);
        if (success)
            *success = 0;
    }
    else if (success)
    {
        *success = 1;
    }

    if (line) gpiod_line_release(line); // Release line only if it was successfully acquired/requested

    return val_int;
#endif
}

// Read GPIO pin transitions (pulses) over a period of time
pulse_count_t gpio_read_pulses(struct gpiod_chip *chip, gpio_info_t *info, int duration_ms)
{
    pulse_count_t result = {0};
    int val, prev_val = -1, success;
    struct timespec start_time, current_time;
    long elapsed_ms;

    if (!chip)
    {
        return result;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    current_time = start_time;

    while (1)
    {
        // Calculate elapsed time
        elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                     (current_time.tv_nsec - start_time.tv_nsec) / 1000000;

        if (elapsed_ms >= duration_ms)
            break;

        val = gpio_get_value(chip, info, &success);
        if (!success)
        {
            result.error = 1;
            return result;
        }

        // If first read or value changed, count transitions
        if (prev_val != -1 && val != prev_val)
        {
            result.count++;
            
            // Count only rising edges (0->1) if we're just counting pulses
            // This avoids double-counting transitions
            if (val == 1)
                result.rising_edges++;
        }

        prev_val = val;
        
        // Small delay to avoid excessive CPU usage
        usleep(100);  // 0.1ms sleep

        clock_gettime(CLOCK_MONOTONIC, &current_time);
    }

    result.duration_ms = elapsed_ms;
    return result;
}
