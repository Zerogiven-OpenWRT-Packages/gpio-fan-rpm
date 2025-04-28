// args.c
// Command-line argument parser for gpio-fan-rpm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpio-fan-rpm.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifndef PKG_TAG
#define PKG_TAG "0.0.0-dev"
#endif

#ifndef PKG_MAINTAINER
#define PKG_MAINTAINER "Unknown"
#endif

#ifndef PKG_LICENSE
#define PKG_LICENSE "Proprietary"
#endif

#ifndef PKG_COPYRIGHT_YEAR
#define PKG_COPYRIGHT_YEAR "2025"
#endif

// Sanitize chip name by stripping /dev/ prefix if present
void sanitize_chip_name(char *chip_name)
{
    const char *prefix = "/dev/";
    size_t prefix_len = strlen(prefix);

    if (strncmp(chip_name, prefix, prefix_len) == 0)
    {
        memmove(chip_name, chip_name + prefix_len, strlen(chip_name + prefix_len) + 1);
    }
}

// Helper to check long or short option match
int is_option(const char *arg, const char *longopt, const char *shortopt)
{
    return (strncmp(arg, longopt, strlen(longopt)) == 0 || strcmp(arg, shortopt) == 0);
}

// Extract value from "--key=value" or return NULL
const char *get_value(const char *arg)
{
    const char *eq = strchr(arg, '=');
    return (eq && *(eq + 1)) ? eq + 1 : NULL;
}

// Parse command-line arguments and return the config structure
config_t parse_arguments(int argc, char *argv[])
{
    config_t cfg = {0};

    // Defaults
    cfg.duration = 2;
    cfg.pulses_per_rev = 2;
    strncpy(cfg.default_chip, "gpiochip0", sizeof(cfg.default_chip) - 1);

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        const char *val = NULL;

        if (strncmp(arg, "--", 2) == 0 && strchr(arg, '='))
        {
            val = get_value(arg);
        }

        if (is_option(arg, "--gpio", "-g"))
        {
            if (!val && i + 1 < argc)
                val = argv[++i];
            if (!val)
                goto error;
            if (cfg.gpio_count >= MAX_GPIOS)
            {
                fprintf(stderr, "Too many GPIOs specified (max %d)\n", MAX_GPIOS);
                exit(EXIT_FAILURE);
            }
            int idx = cfg.gpio_count;
            cfg.gpios[idx].gpio_rel = atoi(val);
            cfg.gpios[idx].pulses_per_rev = -1;
            strncpy(cfg.gpios[idx].chip, cfg.default_chip, sizeof(cfg.gpios[idx].chip) - 1);
            cfg.gpio_count++;
        }
        else if (is_option(arg, "--chip", "-c"))
        {
            if (!val && i + 1 < argc)
                val = argv[++i];
            if (!val)
                goto error;
            strncpy(cfg.default_chip, val, sizeof(cfg.default_chip) - 1);
            sanitize_chip_name(cfg.default_chip);
        }
        else if (is_option(arg, "--duration", "-d"))
        {
            if (!val && i + 1 < argc)
                val = argv[++i];
            if (!val)
                goto error;
            cfg.duration = atoi(val);
        }
        else if (is_option(arg, "--pulses", "-p"))
        {
            if (!val && i + 1 < argc)
                val = argv[++i];
            if (!val)
                goto error;
            cfg.pulses_per_rev = atoi(val);
        }
        else if (is_option(arg, "--numeric", ""))
        {
            cfg.numeric_output = 1;
        }
        else if (is_option(arg, "--json", ""))
        {
            cfg.json_output = 1;
        }
        else if (is_option(arg, "--collectd", ""))
        {
            cfg.collectd_output = 1;
        }
        else if (is_option(arg, "--debug", ""))
        {
            cfg.debug = 1;
        }
        else if (is_option(arg, "--watch", "-w"))
        {
            cfg.watch_mode = 1;
        }
        else if (is_option(arg, "--help", "-h"))
        {
            cfg.show_help = 1;
        }
        else if (is_option(arg, "--version", "-v"))
        {
            cfg.show_version = 1;
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", arg);
            cfg.show_help = 1;
        }

        continue;

    error:
        fprintf(stderr, "Missing or invalid value for option: %s\n", arg);
        cfg.show_help = 1;
    }

    // Apply global pulses_per_rev if not set per-GPIO
    for (int i = 0; i < cfg.gpio_count; i++)
    {
        if (cfg.gpios[i].pulses_per_rev <= 0)
            cfg.gpios[i].pulses_per_rev = cfg.pulses_per_rev;
    }

    if (cfg.debug)
    {
        printf("[DEBUG] Parsed args: duration=%d, pulses_per_rev=%d, gpios=%d\n", cfg.duration, cfg.pulses_per_rev, cfg.gpio_count);
               
        for (int i = 0; i < cfg.gpio_count; i++)
        {
            printf("[DEBUG] GPIO %d: chip=%s, pulses=%d\n",
                   cfg.gpios[i].gpio_rel,
                   cfg.gpios[i].chip,
                   cfg.gpios[i].pulses_per_rev);
        }
    }

    return cfg;
}

// Help output
void print_help(const char *prog)
{
    printf("\nUsage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  --gpio=N, -g N         GPIO number to measure (relative to chip)\n");
    printf("  --chip=NAME, -c NAME   GPIO chip (default: gpiochip0)\n");
    printf("  --duration=SEC, -d SEC Duration to measure (default: 2)\n");
    printf("  --pulses=N, -p N       Pulses per revolution (default: 2)\n");
    printf("  --numeric              Output RPM as numeric only\n");
    printf("  --json                 Output as JSON\n");
    printf("  --collectd             Output in collectd PUTVAL format\n");
    printf("  --debug                Show debug output\n");
    printf("  --watch, -w            Repeat measurements continuously\n");
    printf("  --help, -h             Show this help\n");
    printf("  --version, -v          Show version information\n");
    printf("\n");
    printf("Note:\n");
    printf("  The --duration value is internally split into two phases:\n");
    printf("    • Warmup phase:      duration / 2 (no output)\n");
    printf("    • Measurement phase: duration / 2 (output shown)\n");
    printf("  In --watch mode, the warmup is only performed once (before the loop).\n");
    printf("  Each loop iteration then only uses duration / 2 for measurement.\n\n");
}

void print_version(const char *prog)
{
    printf("%s version %s\n", prog, PKG_TAG);
    printf("Copyright (C) %s %s\n", PKG_COPYRIGHT_YEAR, PKG_MAINTAINER);
    printf("License: %s\n", PKG_LICENSE);
}
