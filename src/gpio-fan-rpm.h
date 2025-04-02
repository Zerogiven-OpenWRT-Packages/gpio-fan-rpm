// gpio-fan-rpm.h
// Shared definitions and structures for gpio-fan-rpm

#ifndef GPIO_FAN_RPM_H
#define GPIO_FAN_RPM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    #define MAX_GPIOS 32
    #define MAX_CHIP_NAME 64

    // Structure for each GPIO pin to monitor
    typedef struct
    {
        int gpio_rel;             // Relative GPIO number
        int gpio_abs;             // Absolute GPIO number (base + relative)
        int base;                 // Base GPIO offset
        char chip[MAX_CHIP_NAME]; // Associated gpiochip name
        int rpm;                  // Measured RPM value
        int pulses_per_rev;       // Number of pulses per revolution
        int valid;                // Measurement status flag
    } gpio_info_t;

    // Global configuration
    typedef struct
    {
        gpio_info_t gpios[MAX_GPIOS];
        int gpio_count;

        // Measurement parameters
        int duration;
        int pulses_per_rev;

        // Output format options
        int numeric_output;
        int json_output;
        int collectd_output;
        int output_quiet;

        // Behavior options
        int debug;
        int watch_mode;
        int show_help;

        // Default chip/base fallback
        int default_base;
        char default_chip[MAX_CHIP_NAME];
    } config_t;

    // Function declarations
    config_t parse_arguments(int argc, char *argv[]);
    void print_help(const char *prog);
    void perform_measurements(config_t *config);
    int read_gpio_base(const char *chip);
    int default_gpio_base(char *chip_buf, size_t len);
    int export_gpio(int gpio_abs);
    void setup_gpio(int gpio_abs);
    void detect_chip_and_base(config_t *cfg);
    int read_gpio(int gpio_abs); // Reads the GPIO pin state

#ifdef __cplusplus
}
#endif

#endif // GPIO_FAN_RPM_H
