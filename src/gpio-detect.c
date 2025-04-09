// gpio-detect.c
// GPIO chip detection using libgpiod v2 (pure C)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpiod.h>
#include "gpio-fan-rpm.h"

#define MAX_CHIPS 32

// Try to find the first available chip if none provided
void detect_chip(config_t *cfg)
{
    if (cfg->default_chip[0] != '\0')
        return;

    for (int i = 0; i < MAX_CHIPS; i++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/dev/gpiochip%d", i);

        struct gpiod_chip *chip = gpiod_chip_open(path);
        if (!chip)
            continue;

        // Save the chip name (e.g., "gpiochip0") without "/dev/"
        const char *name = gpiod_chip_get_path(chip);
        if (name)
        {
            const char *basename = strrchr(name, '/');
            if (basename && strlen(basename + 1) < sizeof(cfg->default_chip))
                strncpy(cfg->default_chip, basename + 1, sizeof(cfg->default_chip) - 1);
        }

        gpiod_chip_close(chip);
        return; // stop after finding the first usable chip
    }

    // fallback if nothing found
    strncpy(cfg->default_chip, "gpiochip0", sizeof(cfg->default_chip) - 1);
}
