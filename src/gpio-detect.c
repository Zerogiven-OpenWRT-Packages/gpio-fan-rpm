// gpio-detect.c
// Auto-detect gpiochip name and base from /sys/class/gpio

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include "gpio-fan-rpm.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"

int read_gpio_base_from_chip(const char *chipname)
{
    char path[128];
    snprintf(path, sizeof(path), SYSFS_GPIO_DIR "/%s/base", chipname);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int base = -1;
    fscanf(f, "%d", &base);
    fclose(f);
    return base;
}

int find_first_gpiochip(char *chip_buf, size_t len, int *base_out)
{
    DIR *dir = opendir(SYSFS_GPIO_DIR);
    if (!dir) return -1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "gpiochip", 8) == 0)
        {
            int base = read_gpio_base_from_chip(entry->d_name);
            if (base >= 0)
            {
                strncpy(chip_buf, entry->d_name, len - 1);
                chip_buf[len - 1] = '\0';
                if (base_out) *base_out = base;
                closedir(dir);
                return 0;
            }
        }
    }

    closedir(dir);
    return -1;
}

int find_chip_by_base(int base, char *chip_buf, size_t len)
{
    DIR *dir = opendir(SYSFS_GPIO_DIR);
    if (!dir) return -1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "gpiochip", 8) == 0)
        {
            int chip_base = read_gpio_base_from_chip(entry->d_name);
            if (chip_base == base)
            {
                strncpy(chip_buf, entry->d_name, len - 1);
                chip_buf[len - 1] = '\0';
                closedir(dir);
                return 0;
            }
        }
    }

    closedir(dir);
    return -1;
}

void detect_chip_and_base(config_t *cfg)
{
    if (cfg->default_chip[0] == 0 && cfg->default_base <= 0)
    {
        find_first_gpiochip(cfg->default_chip, MAX_CHIP_NAME, &cfg->default_base);
    }
    else if (cfg->default_chip[0] != 0 && cfg->default_base <= 0)
    {
        cfg->default_base = read_gpio_base_from_chip(cfg->default_chip);
    }
    else if (cfg->default_chip[0] == 0 && cfg->default_base > 0)
    {
        find_chip_by_base(cfg->default_base, cfg->default_chip, MAX_CHIP_NAME);
    }
}
