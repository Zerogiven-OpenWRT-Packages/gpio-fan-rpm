// gpio-fan-rpm.h
// Shared definitions and structures for gpio-fan-rpm

#ifndef GPIO_FAN_RPM_H
#define GPIO_FAN_RPM_H

#include <stddef.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <gpiod.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_GPIOS 32
#define MAX_CHIP_NAME 64

    // Structure to hold pulse counting results
    typedef struct
    {
        uint32_t count;         // Total number of transitions (both rising and falling edges)
        uint32_t rising_edges;  // Number of rising edges only (0->1 transitions)
        uint32_t duration_ms;   // Actual measurement duration in milliseconds
        int error;              // Error flag (1 if error occurred, 0 otherwise)
    } pulse_count_t;

    // GPIO pin information
    typedef struct
    {
        char chip[32];          // chip name (e.g. "gpiochip0")
        int gpio_rel;           // relative GPIO pin number on the chip
        int pulses_per_rev;     // Pulses per revolution for this GPIO
        int rpm;                // Measured RPM value
        int valid;              // Flag to indicate valid measurement
    } gpio_info_t;

    // Global configuration for the application
    typedef struct
    {
        char default_chip[32];  // Default GPIO chip name
        int debug;              // Debug level
        int daemon_mode;        // Run as daemon
        char socket_path[128];  // Socket path for daemon
        int interval;           // Measurement interval in seconds
        int duration;           // Measurement duration in seconds
        gpio_info_t gpios[MAX_GPIOS]; // GPIO pin definitions
        int gpio_count;               // Number of GPIOs configured

        // Measurement parameters
        int pulses_per_rev; // Pulses per revolution (global fallback)

        // Output format options
        int numeric_output;  // --numeric
        int json_output;     // --json
        int collectd_output; // --collectd

        // Behavior options
        int watch_mode;    // --watch
        int show_help;     // --help
        int show_version;  // --version
    } config_t;

    // Arguments for each thread used during edge-based RPM measurement
    typedef struct
    {
        gpio_info_t *info;  // Pointer to gpio_info for this thread
        int duration;       // Measurement duration
        int pulses_per_rev; // Pulses per revolution (individual)
        int rpm_out;        // Output value for measured RPM
        int success;        // Flag to indicate success or error
        int debug;          // Enable debug output
    } edge_thread_args_t;

    // Function declarations

    // Argument parsing
    config_t parse_arguments(int argc, char *argv[]);
    void print_help(const char *prog);
    void print_version(const char *prog);

    // Measurement core
    void perform_measurements(config_t *config, int skip_output);

    // RPM measurement via polling (legacy fallback)
    int gpio_get_value(struct gpiod_chip *chip, gpio_info_t *info, int *success);

    // RPM measurement using edge detection and threads
    float measure_rpm_edge(const char *chip_name, int pin, int debug_level);

    // Chip auto-detection via libgpiod
    void detect_chip(config_t *cfg);

    // GPIO operations
    struct gpiod_chip *gpio_open_chip(const char *chip_name);
    void gpio_close_chip(struct gpiod_chip *chip);
    pulse_count_t gpio_read_pulses(struct gpiod_chip *chip, gpio_info_t *info, int duration_ms);

#ifdef __cplusplus
}
#endif

#endif // GPIO_FAN_RPM_H
