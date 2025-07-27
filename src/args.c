/**
 * @file args.c
 * @brief Command-line argument parsing and help functionality
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
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

void print_usage(const char *prog) {
    printf("\n%s %s\n", prog, PKG_TAG_STR);
    printf("High-precision fan RPM measurement using GPIO edge detection\n\n");
    
    printf("USAGE\n");
    printf("  %s [OPTIONS]\n\n", prog);
    
    printf("REQUIRED ARGUMENTS\n");
    printf("  -g, --gpio=N           GPIO pin number to measure (can be repeated)\n\n");
    
    printf("OPTIONAL ARGUMENTS\n");
    printf("  -c, --chip=NAME        GPIO chip name (default: auto-detect)\n");
    printf("  -d, --duration=SEC     Measurement duration in seconds (default: 2)\n");
    printf("  -p, --pulses=N         Pulses per revolution (default: 2)\n");
    printf("  -w, --watch            Continuous monitoring mode\n");
    printf("  -h, --help             Show this help message\n");
    printf("  -v, --version          Show version information\n\n");
    
    printf("OUTPUT FORMATS\n");
    printf("  -n, --numeric          Output RPM as numeric value only\n");
    printf("  -j, --json             Output as JSON object/array\n");
    printf("  --collectd             Output in collectd PUTVAL format\n");
    printf("  --debug                Show detailed measurement information\n\n");
    
    printf("MEASUREMENT BEHAVIOR\n");
    printf("  The --duration value represents the total time per measurement cycle.\n");
    printf("  Each measurement is split into two phases:\n");
    printf("    • Warmup phase:      duration/2 (stabilization, no output)\n");
    printf("    • Measurement phase: duration/2 (actual measurement)\n");
    printf("  In --watch mode, warmup occurs only once before the loop.\n\n");
    
    printf("EXAMPLES\n");
    printf("  %s --gpio=17                    # Basic single measurement\n", prog);
    printf("  %s --gpio=17 --pulses=4         # 4-pulse fan (typical for PC fans)\n", prog);
    printf("  %s --gpio=17 --duration=4 --watch # Continuous monitoring\n", prog);
    printf("  %s --gpio=17 --json             # JSON output for scripts\n", prog);
    printf("  %s --gpio=17 --gpio=18 --json   # Multiple fans as JSON array\n", prog);
    printf("  %s --gpio=17 --collectd         # collectd monitoring format\n", prog);
    printf("  RPM=$(%s --gpio=17 --numeric)   # Capture RPM in shell script\n", prog);
    printf("\n");
    
    printf("TROUBLESHOOTING\n");
    printf("  • Ensure GPIO pin is connected to fan tachometer wire\n");
    printf("  • Run as root if permission denied: sudo %s --gpio=17\n", prog);
    printf("  • Use --debug to see detailed measurement information\n");
    printf("  • Verify --pulses matches your fan specification\n\n");
    
    printf("For more information, see: https://github.com/Zerogiven-OpenWRT-Packages/gpio-fan-rpm\n\n");
}

int parse_arguments(int argc, char **argv, int **gpios, size_t *ngpio, 
                   char **chipname, int *duration, int *pulses, 
                   int *debug, int *watch, output_mode_t *mode) {
    int opt;
    
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
            *gpios = realloc(*gpios, (*ngpio + 1) * sizeof(**gpios));
            if (!*gpios) {
                fprintf(stderr, "Error: memory allocation failed\n");
                return -1;
            }
            (*gpios)[*ngpio] = atoi(optarg);
            (*ngpio)++;
            break;
        case 'c': 
            *chipname = strdup(optarg); 
            break;
        case 'd': 
            *duration = atoi(optarg); 
            break;
        case 'p': 
            *pulses = atoi(optarg); 
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
            printf("%s %s\n", argv[0], PKG_TAG_STR); 
            printf("Build: %s %s\n", __DATE__, __TIME__);
            printf("libgpiod: %s\n", LIBGPIOD_VERSION_STR);
            return 1; // Special return to indicate version
        default: 
            print_usage(argv[0]); 
            return -1;
        }
    }
    
    return 0;
}

int validate_arguments(int *gpios, size_t ngpio, int duration, int pulses) {
    if (ngpio == 0) {
        fprintf(stderr, "Error: at least one --gpio required\n");
        return -1;
    }
    
    // Validate GPIO numbers
    for (size_t i = 0; i < ngpio; i++) {
        if (gpios[i] < 0 || gpios[i] > 999) {
            fprintf(stderr, "Error: GPIO %d is out of valid range (0-999)\n", gpios[i]);
            return -1;
        }
    }
    
    // Check for duplicate GPIOs
    for (size_t i = 0; i < ngpio; i++) {
        for (size_t j = i + 1; j < ngpio; j++) {
            if (gpios[i] == gpios[j]) {
                fprintf(stderr, "Error: GPIO %d specified multiple times\n", gpios[i]);
                return -1;
            }
        }
    }
    
    // Validate duration
    if (duration < 1 || duration > 3600) {
        fprintf(stderr, "Error: duration must be between 1 and 3600 seconds\n");
        return -1;
    }
    
    // Validate pulses
    if (pulses < 1 || pulses > 100) {
        fprintf(stderr, "Error: pulses must be between 1 and 100\n");
        return -1;
    }
    
    return 0;
} 