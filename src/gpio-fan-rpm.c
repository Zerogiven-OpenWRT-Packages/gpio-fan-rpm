// gpio-fan-rpm.c
// Modular and well-commented fan RPM measurement utility for OpenWRT
// Author: Open Source Community
// License: MIT

#include <stdio.h>
#include <unistd.h>
#include "gpio-fan-rpm.h"

int main(int argc, char *argv[])
{
    config_t config = parse_arguments(argc, argv);

    if (config.show_help)
    {
        print_help(argv[0]);
        return 0;
    }

    if (config.gpio_count == 0)
    {
        fprintf(stderr, "Error: At least one GPIO is required.\n\n");
        print_help(argv[0]);
        return 1;
    }

    int warmup_time = config.duration / 2;
    int measure_time = config.duration - warmup_time;

    // Chip autodetect falls leer
    if (config.default_chip[0] == '\0')
    {
        detect_chip(&config);
        if (config.debug)
        {
            fprintf(stderr, "[DEBUG] auto-detected chip: %s\n", config.default_chip);
        }
    }

    for (int i = 0; i < config.gpio_count; i++)
    {
        if (config.gpios[i].pulses_per_rev <= 0)
            config.gpios[i].pulses_per_rev = config.pulses_per_rev;

        if (config.gpios[i].chip[0] == '\0')
            snprintf(config.gpios[i].chip, sizeof(config.gpios[i].chip), "%s", config.default_chip);

        if (config.debug)
        {
            printf("[DEBUG] GPIO %d: chip=%s, pulses/rev=%d\n",
                   config.gpios[i].gpio_rel,
                   config.gpios[i].chip,
                   config.gpios[i].pulses_per_rev);
        }
    }

    if (!config.output_quiet &&
        !config.numeric_output &&
        !config.json_output &&
        !config.collectd_output)
    {
        printf("gpio-fan-rpm start measuring (chip: %s)\n", config.default_chip);
        printf("Measuring for %d second(s)...\n", warmup_time + measure_time);
    }

    // Warmup
    config.duration = warmup_time;
    perform_measurements(&config);
    sleep(warmup_time);

    config.duration = measure_time;

    if (config.watch_mode)
    {
        int first_loop = 1;

        while (1)
        {
            if (!first_loop &&
                !config.output_quiet &&
                !config.numeric_output &&
                !config.json_output &&
                !config.collectd_output)
            {
                printf("Measuring for %d second(s)...\n", config.duration);
            }

            perform_measurements(&config);

            if (!config.output_quiet &&
                !config.numeric_output &&
                !config.json_output &&
                !config.collectd_output)
            {
                printf("---\n");
            }

            first_loop = 0;
            sleep(config.duration);
        }
    }
    else
    {
        perform_measurements(&config);
    }

    return 0;
}
