// gpio.c
// GPIO setup and value reading using libgpiod v2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpiod.h>
#include "gpio-fan-rpm.h"

// Read value using libgpiod v2
int gpio_get_value(gpio_info_t *info)
{
    if (!info || info->gpio_rel < 0 || info->chip[0] == 0)
        return -1;

    char chip_path[128];

    // Prepend "/dev/" to the chip name if not already included
    if (strncmp(info->chip, "/dev/", 5) != 0)
        snprintf(chip_path, sizeof(chip_path), "/dev/%s", info->chip);
    else
        snprintf(chip_path, sizeof(chip_path), "%s", info->chip);

    // Open chip by path
    struct gpiod_chip *chip = gpiod_chip_open(chip_path);
    if (!chip)
        return -1;

    // Create line settings for input
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        gpiod_chip_close(chip);
        return -1;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

    // Configure the line
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return -1;
    }

    unsigned int offset = info->gpio_rel;
    if (gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings) < 0) {
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return -1;
    }

    // Request the line
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    if (!req_cfg) {
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return -1;
    }

    gpiod_request_config_set_consumer(req_cfg, "gpio-fan-rpm");

    struct gpiod_line_request *request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    if (!request) {
        gpiod_chip_close(chip);
        return -1;
    }

    enum gpiod_line_value value = gpiod_line_request_get_value(request, info->gpio_rel);
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);

    if (value == GPIOD_LINE_VALUE_ERROR)
        return -1;

    return value == GPIOD_LINE_VALUE_ACTIVE ? 1 : 0;
}
