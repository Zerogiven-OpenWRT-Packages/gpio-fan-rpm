// gpio.c
// GPIO setup and value reading using libgpiod v1/v2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <poll.h>
#include "gpio.h"
#include "gpio_compat.h"
#include "gpio-fan-rpm.h"

struct gpio_pin {
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int fd;
};

struct gpio_pin *gpio_open(const char *chipname, int pin, const char *consumer)
{
    struct gpio_pin *gpio = calloc(1, sizeof(struct gpio_pin));
    if (!gpio) {
        fprintf(stderr, "Error: Out of memory.\n");
        return NULL;
    }

    gpio->chip = gpiod_chip_open_lookup(chipname);
    if (!gpio->chip) {
        fprintf(stderr, "Error: Cannot open GPIO chip '%s': %s\n", 
                chipname, strerror(errno));
        gpio_close(gpio);
        return NULL;
    }

    // Use our compatibility layer to get the line
    gpio->line = gpio_compat_get_line(gpio->chip, pin);
    if (!gpio->line) {
        fprintf(stderr, "Error: Cannot get line %d from chip '%s': %s\n", 
                pin, chipname, strerror(errno));
        gpio_close(gpio);
        return NULL;
    }

    // Use our compatibility layer to request edge events
    int ret = gpio_compat_request_events(gpio->line, consumer, 1);  // 1 = falling only
    if (ret < 0) {
        fprintf(stderr, "Error: Cannot request falling edge events on line %d: %s\n", 
                pin, strerror(errno));
        gpio_close(gpio);
        return NULL;
    }

    // Use our compatibility layer to get the event file descriptor
    gpio->fd = gpio_compat_get_fd(gpio->line);
    if (gpio->fd < 0) {
        fprintf(stderr, "Error: Cannot get line event file descriptor: %s\n", 
                strerror(errno));
        gpio_close(gpio);
        return NULL;
    }

    return gpio;
}

void gpio_close(struct gpio_pin *gpio)
{
    if (!gpio) return;

    if (gpio->line) {
        gpio_compat_release_line(gpio->line);
    }

    if (gpio->chip) {
        gpiod_chip_close(gpio->chip);
    }

    free(gpio);
}

int gpio_get_fd(struct gpio_pin *gpio)
{
    return gpio ? gpio->fd : -1;
}

int gpio_wait_for_event(struct gpio_pin *gpio, int timeout_ms)
{
    if (!gpio || gpio->fd < 0) return -1;

    struct pollfd pfd;
    pfd.fd = gpio->fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0) {
        fprintf(stderr, "Error: Failed to poll GPIO: %s\n", strerror(errno));
        return -1;
    }

    if (ret == 0) {
        // Timeout
        return 0;
    }

    if (pfd.revents & POLLIN) {
        struct gpio_compat_event event;
        
        // Use our compatibility layer to read the event
        if (gpio_compat_read_event(gpio->line, &event) < 0) {
            fprintf(stderr, "Error: Failed to read GPIO event: %s\n", strerror(errno));
            return -1;
        }
        
        // Got an event (we only request falling edges)
        return 1;
    }

    return 0;
}

const char *gpio_get_chipname(struct gpio_pin *gpio)
{
    if (!gpio || !gpio->chip) return NULL;
    
    // Use our compatibility layer to get the chip path
    const char *path = gpio_compat_get_chip_path(gpio->chip);
    if (!path) return "unknown";
    
    // Return just the filename portion
    const char *name = strrchr(path, '/');
    return name ? name + 1 : path;
}

// Read GPIO pin transitions (pulses) over a period of time
pulse_count_t gpio_read_pulses(struct gpio_pin *gpio, int duration_ms)
{
    pulse_count_t result = {0};
    int val, prev_val = -1, success;
    struct timespec start_time, current_time;
    long elapsed_ms;

    if (!gpio)
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

        val = gpio_compat_get_value(gpio->line, &success);
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
