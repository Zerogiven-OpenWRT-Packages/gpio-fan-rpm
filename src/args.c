/**
 * @file args.c
 * @brief Command-line argument parsing and help functionality
 * @author CSoellinger
 * @date 2025
 * @license LGPL-3.0-or-later
 * 
 * This module handles command-line argument parsing, validation,
 * and help output for the gpio-fan-rpm utility.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <uci.h>
#include "args.h"

#ifndef PKG_TAG
#define PKG_TAG_STR "unknown"
#else
#define _STR(x) #x
#define STR(x) _STR(x)
#define PKG_TAG_STR STR(PKG_TAG)
#endif

#ifndef LIBGPIOD_VERSION
#define LIBGPIOD_VERSION_STR "unknown"
#else
#define LIBGPIOD_VERSION_STR STR(LIBGPIOD_VERSION)
#endif

/**
 * @brief Safely convert string to integer with error checking
 *
 * @param str String to convert
 * @param result Pointer to store result
 * @return int 0 on success, -1 on error
 */
static int safe_str_to_int(const char *str, int *result) {
    if (!str || !result) return -1;

    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    // Check for various error conditions
    if (errno == ERANGE || val > INT_MAX || val < INT_MIN) {
        return -1; // Overflow/underflow
    }
    if (endptr == str || *endptr != '\0') {
        return -1; // No conversion or trailing characters
    }

    *result = (int)val;
    return 0;
}

// Function to load default values from UCI configuration and environment variables
int load_uci_defaults(int *duration, int *pulses) {
    // Check environment variables first (highest precedence)
    const char *env_duration = getenv("GPIO_FAN_RPM_DURATION");
    const char *env_pulses = getenv("GPIO_FAN_RPM_PULSES");
    const char *env_debug = getenv("DEBUG");
    
    if (env_duration) {
        int temp;
        if (safe_str_to_int(env_duration, &temp) == 0) {
            *duration = temp;
        }
        // Silently ignore invalid env values
    }

    if (env_pulses) {
        int temp;
        if (safe_str_to_int(env_pulses, &temp) == 0) {
            *pulses = temp;
        }
        // Silently ignore invalid env values
    }
    
    // Return debug flag if set
    if (env_debug && (strcmp(env_debug, "1") == 0 || strcmp(env_debug, "true") == 0)) {
        return 1; // Debug enabled
    }
    
    // Then load UCI configuration (lower precedence)
    struct uci_context *ctx = uci_alloc_context();
    if (!ctx) return -1;
    
    struct uci_package *pkg = NULL;
    int ret = uci_load(ctx, "gpio-fan-rpm", &pkg);
    if (ret != UCI_OK) {
        uci_free_context(ctx);
        return 0; // No config file, use defaults
    }
    
    struct uci_section *s = uci_lookup_section(ctx, pkg, "defaults");
    if (s) {
        const char *val;
        int temp;

        val = uci_lookup_option_string(ctx, s, "duration");
        if (val && safe_str_to_int(val, &temp) == 0) {
            *duration = temp;
        }

        val = uci_lookup_option_string(ctx, s, "pulses");
        if (val && safe_str_to_int(val, &temp) == 0) {
            *pulses = temp;
        }
    }
    
    uci_free_context(ctx);
    return 0; // Debug disabled
}

void print_usage(const char *prog) {
    printf("\n");
    printf("Usage: %s [OPTIONS] --gpio=N [--gpio=N...]\n\n", prog);
    printf("Measure fan RPM using GPIO edge detection.\n\n");
    
    printf("Required:\n");
    printf("  -g, --gpio=N           GPIO pin number to measure (can be repeated)\n\n");
    
    printf("Options:\n");
    printf("  -c, --chip=NAME        GPIO chip name (default: auto-detect)\n");
    printf("  -d, --duration=SEC     Measurement duration in seconds (default: 2, min: 2)\n");
    printf("  -p, --pulses=N         Pulses per revolution (default: 4)\n");
    printf("  -w, --watch            Continuous monitoring mode\n");
    printf("  -n, --numeric          Output RPM as numeric value only\n");
    printf("  -j, --json             Output as JSON object/array\n");
    printf("  --collectd             Output in collectd PUTVAL format\n");
    printf("  --debug                Show detailed measurement information\n");
    printf("  -h, --help             Show this help message\n");
    printf("  -v, --version          Show version information\n\n");
    
    printf("Watch Mode:\n");
    printf("  In watch mode, press 'q' to quit gracefully or Ctrl+C to interrupt.\n");
    printf("\n");

    printf("Examples:\n");
    printf("  %s --gpio=17                    # Basic measurement\n", prog);
    printf("  %s --gpio=17 --pulses=4         # 4-pulse fan\n", prog);
    printf("  %s --gpio=17 --duration=4 --watch # Continuous monitoring\n", prog);
    printf("  %s --gpio=17 --json             # JSON output\n", prog);
    printf("  %s --gpio=17 --gpio=18 --json   # Multiple fans\n", prog);
    printf("  RPM=$(%s --gpio=17 --numeric)   # Capture in variable\n", prog);
    printf("\n");
    
    printf("For more information, see: https://github.com/Zerogiven-OpenWRT-Packages/gpio-fan-rpm\n");
    printf("\n");
}

int parse_arguments(int argc, char **argv, int **gpios, size_t *ngpio, 
                   char **chipname, int *duration, int *pulses, 
                   int *debug, int *watch, output_mode_t *mode) {
    int opt;
    
    // Load UCI defaults first
    int env_debug = load_uci_defaults(duration, pulses);
    if (env_debug > 0) {
        *debug = 1;
    }
    
    // Set reasonable limits to prevent resource exhaustion
    if (*duration > 3600) {
        fprintf(stderr, "Warning: Duration %d seconds is very long, limiting to 3600\n", *duration);
        *duration = 3600;
    }
    
    if (*ngpio > 10) {
        fprintf(stderr, "Warning: Too many GPIOs (%zu), limiting to 10\n", *ngpio);
        *ngpio = 10;
    }
    
    struct option longopts[] = {
        {"gpio", required_argument, 0, 'g'},
        {"chip", required_argument, 0, 'c'},
        {"duration", required_argument, 0, 'd'},
        {"pulses", required_argument, 0, 'p'},
        {"numeric", no_argument, 0, 'n'},
        {"json", no_argument, 0, 'j'},
        {"collectd", no_argument, 0, 'C'},
        {"debug", no_argument, 0, 'D'},
        {"watch", no_argument, 0, 'w'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "g:c:d:p:njCDwhv", longopts, NULL)) != -1) {
        switch (opt) {
        case 'g':
            // Validate GPIO number format
            if (!optarg || *optarg == '\0') {
                fprintf(stderr, "\nError: --gpio requires a number\n\n");
                return -1;
            }
            
            int gpio_val;
            if (safe_str_to_int(optarg, &gpio_val) != 0) {
                fprintf(stderr, "\nError: GPIO pin must be a valid number, got '%s'\n\n", optarg);
                fprintf(stderr, "Try: %s --help\n\n", argv[0]);
                return -1;
            }
            if (gpio_val < 0 || gpio_val > 999) {
                fprintf(stderr, "\nError: GPIO pin %d is out of valid range (0-999)\n\n", gpio_val);
                fprintf(stderr, "Try: %s --help\n\n", argv[0]);
                return -1;
            }

            int *temp = realloc(*gpios, (*ngpio + 1) * sizeof(**gpios));
            if (!temp) {
                fprintf(stderr, "\nError: memory allocation failed\n\n");
                free(*gpios);
                *gpios = NULL;
                return -1;
            }
            *gpios = temp;
            (*gpios)[*ngpio] = gpio_val;
            (*ngpio)++;
            break;
        case 'c':
            *chipname = strdup(optarg);
            if (!*chipname) {
                fprintf(stderr, "\nError: memory allocation failed\n\n");
                free(*gpios);
                *gpios = NULL;
                return -1;
            }
            break;
        case 'd':
            if (!optarg || *optarg == '\0') {
                fprintf(stderr, "\nError: --duration requires a number\n\n");
                fprintf(stderr, "Try: %s --help\n\n", argv[0]);
                return -1;
            }
            if (safe_str_to_int(optarg, duration) != 0) {
                fprintf(stderr, "\nError: --duration must be a valid number, got '%s'\n\n", optarg);
                fprintf(stderr, "Try: %s --help\n\n", argv[0]);
                return -1;
            }
            if (*duration < 2) {
                fprintf(stderr, "\nError: duration must be at least 2 seconds for accurate measurements\n");
                fprintf(stderr, "  Minimum 2s allows for 1s warmup + 1s measurement\n");
                fprintf(stderr, "  For quick tests, try: %s --duration=2\n\n", argv[0]);
                return -1;
            }
            if (*duration > 3600) {
                fprintf(stderr, "\nError: duration must be between 2 and 3600 seconds\n\n");
                fprintf(stderr, "Try: %s --help\n\n", argv[0]);
                return -1;
            }
            break;
        case 'p':
            if (!optarg || *optarg == '\0') {
                fprintf(stderr, "\nError: --pulses requires a number\n\n");
                fprintf(stderr, "Try: %s --help\n\n", argv[0]);
                return -1;
            }
            if (safe_str_to_int(optarg, pulses) != 0) {
                fprintf(stderr, "\nError: --pulses must be a valid number, got '%s'\n\n", optarg);
                fprintf(stderr, "Try: %s --help\n\n", argv[0]);
                return -1;
            }
            if (*pulses < 1 || *pulses > 100) {
                fprintf(stderr, "\nError: pulses must be between 1 and 100\n\n");
                fprintf(stderr, "Try: %s --help\n\n", argv[0]);
                return -1;
            }
            break;
        case 'n': 
            *mode = MODE_NUMERIC; 
            break;
        case 'j': 
            *mode = MODE_JSON; 
            break;
        case 'C': 
            *mode = MODE_COLLECTD; 
            break;
        case 'D': 
            *debug = 1; 
            break;
        case 'w': 
            *watch = 1; 
            break;
        case 'h': 
            print_usage(argv[0]); 
            return 1; // Special return to indicate help
        case 'v': 
            printf("\n");
            printf("%s: %s\n", argv[0], PKG_TAG_STR); 
            printf("Build:        %s %s\n", __DATE__, __TIME__);
            printf("libgpiod:     %s (detected at compile time)\n", LIBGPIOD_VERSION_STR);
            printf("\n");

            return 1; // Special return to indicate version
        default: 
            print_usage(argv[0]); 
            return -1;
        }
    }
    
    return 0;
}

int validate_arguments(int *gpios, size_t ngpio, int duration, int pulses, const char *prog) {
    (void)duration;  // Unused parameter - validation done during parsing
    (void)pulses;    // Unused parameter - validation done during parsing
    
    if (ngpio == 0) {
        fprintf(stderr, "\nError: at least one --gpio required\n\n");
        fprintf(stderr, "Try: %s --help\n\n", prog);
        return -1;
    }
    
    // Check for duplicate GPIOs
    for (size_t i = 0; i < ngpio; i++) {
        for (size_t j = i + 1; j < ngpio; j++) {
            if (gpios[i] == gpios[j]) {
                fprintf(stderr, "\nError: GPIO pin %d specified multiple times\n\n", gpios[i]);
                fprintf(stderr, "Try: %s --help\n\n", prog);
                return -1;
            }
        }
    }
    
    return 0;
} 