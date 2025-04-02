// args.c
// Handles command-line argument parsing for gpio-fan-rpm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "gpio-fan-rpm.h"

// Helper to split "gpio:chip" or just "gpio"
void parse_gpio_argument(const char *arg, gpio_info_t *info, int default_pulses, const char *default_chip)
{
    if (!arg || !info) return;

    info->gpio_rel = -1;
    info->base = -1;
    info->pulses_per_rev = default_pulses;
    info->valid = 1;

    const char *colon = strchr(arg, ':');
    if (colon) {
        char gpio_part[16] = {0};
        size_t len = colon - arg;
        if (len >= sizeof(gpio_part)) len = sizeof(gpio_part) - 1;
        strncpy(gpio_part, arg, len);
        info->gpio_rel = atoi(gpio_part);
        strncpy(info->chip, colon + 1, MAX_CHIP_NAME - 1);
        info->chip[MAX_CHIP_NAME - 1] = '\0';
    } else {
        info->gpio_rel = atoi(arg);
        if (default_chip && strlen(default_chip) > 0) {
            strncpy(info->chip, default_chip, MAX_CHIP_NAME - 1);
            info->chip[MAX_CHIP_NAME - 1] = '\0';
        } else {
            info->chip[0] = '\0';
        }
    }
}

config_t parse_arguments(int argc, char *argv[])
{
    config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.duration = 2;
    cfg.pulses_per_rev = 2;
    cfg.default_base = -1;
    cfg.default_chip[0] = 0;

    static struct option long_opts[] = {
        {"gpio", required_argument, 0, 'g'},
        {"gpio-base", required_argument, 0, 'b'},
        {"gpiochip", required_argument, 0, 'c'},
        {"duration", required_argument, 0, 'd'},
        {"pulses-per-rev", required_argument, 0, 'p'},
        {"numeric", no_argument, 0, 'n'},
        {"json", no_argument, 0, 'j'},
        {"collectd", no_argument, 0, 1000},
        {"watch", no_argument, 0, 'w'},
        {"debug", no_argument, 0, 1001},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    opterr = 0;
    int opt, longidx;
    while ((opt = getopt_long(argc, argv, "g:b:c:d:p:njwh", long_opts, &longidx)) != -1)
    {
        switch (opt)
        {
        case 'g':
            if (cfg.gpio_count < MAX_GPIOS)
            {
                parse_gpio_argument(optarg, &cfg.gpios[cfg.gpio_count],
                                    cfg.pulses_per_rev, cfg.default_chip);
                cfg.gpio_count++;
            }
            break;
        case 'b':
            cfg.default_base = atoi(optarg);
            break;
        case 'c':
            strncpy(cfg.default_chip, optarg, MAX_CHIP_NAME - 1);
            cfg.default_chip[MAX_CHIP_NAME - 1] = '\0';
            cfg.default_base = read_gpio_base(cfg.default_chip);
            break;
        case 'd':
            cfg.duration = atoi(optarg);
            break;
        case 'p':
            cfg.pulses_per_rev = atoi(optarg);
            break;
        case 'n':
            cfg.numeric_output = 1;
            break;
        case 'j':
            cfg.json_output = 1;
            break;
        case 1000:
            cfg.collectd_output = 1;
            break;
        case 'w':
            cfg.watch_mode = 1;
            break;
        case 1001:
            cfg.debug = 1;
            break;
        case 'h':
            cfg.show_help = 1;
            break;
        }
    }

    // Auto-detect chip/base fallback if missing
    if (!cfg.show_help) detect_chip_and_base(&cfg);

    for (int i = optind; i < argc; i++)
    {
        if (cfg.gpio_count < MAX_GPIOS)
        {
            parse_gpio_argument(argv[i], &cfg.gpios[cfg.gpio_count],
                                cfg.pulses_per_rev, cfg.default_chip);
            cfg.gpio_count++;
        }
    }

    // Auto-detect chip + base if still missing and not just --help
    if ((cfg.default_chip[0] == 0 || cfg.default_base <= 0) && !cfg.show_help)
    {
        cfg.default_base = default_gpio_base(cfg.default_chip, MAX_CHIP_NAME);
        if (cfg.debug)
        {
            fprintf(stderr, "[DEBUG] auto-detected chip: %s (base %d)\n",
                    cfg.default_chip, cfg.default_base);
        }
    }

    for (int i = 0; i < cfg.gpio_count; i++)
    {
        if (cfg.gpios[i].chip[0] == 0)
        {
            strncpy(cfg.gpios[i].chip, cfg.default_chip, MAX_CHIP_NAME - 1);
            cfg.gpios[i].chip[MAX_CHIP_NAME - 1] = '\0';
        }
        if (cfg.gpios[i].base <= 0)
        {
            cfg.gpios[i].base = cfg.default_base;
        }
    }

    return cfg;
}

void print_help(const char *prog)
{
    if (!prog) prog = "gpio-fan-rpm";
    printf("Usage: %s [<gpio>[:chip]]... [options]\n\n", prog);
    printf("Options:\n");
    printf("  -g, --gpio=<n>            Relative GPIO number (can repeat)\n");
    printf("      --gpio=n:chip         GPIO number with specific chip\n");
    printf("  -b, --gpio-base=<n>       Manually set GPIO base\n");
    printf("  -c, --gpiochip=<name>     Set default GPIO chip name\n");
    printf("  -d, --duration=<s>        Duration per measurement (default: 2)\n");
    printf("  -p, --pulses-per-rev=<n>  Pulses per revolution (default: 2)\n");
    printf("  -n, --numeric             Output only RPM numbers (per line)\n");
    printf("  -j, --json                Output RPM in JSON format\n");
    printf("      --collectd            Output in collectd Exec format\n");
    printf("  -w, --watch               Repeat measurement continuously\n");
    printf("      --debug               Show debug output\n");
    printf("  -h, --help                Show this help message\n");
}
