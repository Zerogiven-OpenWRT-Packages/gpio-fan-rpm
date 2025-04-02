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

    if (config.show_help || config.gpio_count == 0)
    {
        if (config.gpio_count == 0 && !config.show_help)
        {
            fprintf(stderr, "Error: At least one GPIO is required.\n\n");
        }

        print_help(argv[0]);

        return config.gpio_count == 0 ? 1 : 0;
    }

    if (!config.output_quiet && !config.numeric_output &&
        !config.json_output && !config.collectd_output)
    {
        printf("gpio-fan-rpm starting (default base: %d)\n", config.default_base);
    }

    if (config.watch_mode)
    {
        while (1)
        {
            if (!config.numeric_output && !config.json_output && !config.collectd_output)
                printf("---\n");

            perform_measurements(&config);

            if (!config.numeric_output && !config.json_output && !config.collectd_output)
                fflush(stdout); // flush for line overwrite

            sleep(config.duration);
        }

        return 0;
    }

    perform_measurements(&config);

    return 0;
}
