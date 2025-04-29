// gpio-detect.c
// GPIO chip detection using libgpiod v2 (pure C)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gpiod.h>

#include "gpio-fan-rpm.h"

// Forward declaration for compatibility function if not defined
#ifndef gpiod_chip_get_path
const char *gpiod_chip_get_path(struct gpiod_chip *chip);
#endif

#define MAX_CHIPS 32

// Try to find the first available chip if none provided
void detect_chip(config_t *cfg)
{
    if (cfg->default_chip[0] != '\0')
        return;

    // First try direct device path discovery
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
        
        if (cfg->debug)
            fprintf(stderr, "[DEBUG] Auto-detected chip: %s\n", cfg->default_chip);
            
        return; // stop after finding the first usable chip
    }

    // If no chips found by number, try sysfs mechanism
    // This can help with certain kernel configurations
    FILE *fp = popen("ls -1 /sys/class/gpio/gpiochip* 2>/dev/null | head -n 1", "r");
    if (fp != NULL) {
        char path[256] = {0};
        if (fgets(path, sizeof(path), fp) != NULL) {
            // Remove trailing newline if present
            size_t len = strlen(path);
            if (len > 0 && path[len-1] == '\n')
                path[len-1] = '\0';
                
            // Extract the basename (gpiochipN)
            const char *basename = strrchr(path, '/');
            if (basename && strlen(basename + 1) < sizeof(cfg->default_chip)) {
                strncpy(cfg->default_chip, basename + 1, sizeof(cfg->default_chip) - 1);
                if (cfg->debug)
                    fprintf(stderr, "[DEBUG] Auto-detected chip via sysfs: %s\n", cfg->default_chip);
                pclose(fp);
                return;
            }
        }
        pclose(fp);
    }

    // fallback if nothing found
    strncpy(cfg->default_chip, "gpiochip0", sizeof(cfg->default_chip) - 1);
    if (cfg->debug)
        fprintf(stderr, "[DEBUG] No chip detected, using fallback: %s\n", cfg->default_chip);
}
