// gpio-fan-rpm.h
// Shared definitions and structures for gpio-fan-rpm

#ifndef GPIO_FAN_RPM_H
#define GPIO_FAN_RPM_H

#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_GPIOS 32
#define MAX_CHIP_NAME 64

    // Structure for each GPIO pin to monitor
    typedef struct
    {
        int gpio_rel;             // Relative GPIO number (offset on chip)
        char chip[MAX_CHIP_NAME]; // Associated gpiochip name (e.g. "gpiochip0")
        int rpm;                  // Measured RPM value
        int pulses_per_rev;       // Number of pulses per revolution
        int valid;                // Measurement status flag
    } gpio_info_t;

    // Global configuration for the application
    typedef struct
    {
        gpio_info_t gpios[MAX_GPIOS]; // GPIO pin definitions
        int gpio_count;               // Number of GPIOs configured

        // Measurement parameters
        int duration;       // Measurement duration in seconds
        int pulses_per_rev; // Pulses per revolution (global fallback)

        // Output format options
        int numeric_output;  // --numeric
        int json_output;     // --json
        int collectd_output; // --collectd
        int output_quiet;    // --quiet

        // Behavior options
        int debug;      // --debug
        int watch_mode; // --watch
        int show_help;  // --help

        // Default chip fallback (used if not specified per-GPIO)
        char default_chip[MAX_CHIP_NAME];
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

    // Measurement core
    void perform_measurements(config_t *config);

    // RPM measurement via polling (legacy fallback)
    int gpio_get_value(gpio_info_t *info);

    // RPM measurement using edge detection and threads
    int measure_rpm_edge(gpio_info_t *infos, int count, int duration, int debug);

    // Chip auto-detection via libgpiod
    void detect_chip(config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif // GPIO_FAN_RPM_H
