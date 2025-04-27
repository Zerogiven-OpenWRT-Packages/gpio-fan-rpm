// gpio.c
// GPIO setup and value reading using libgpiod v1/v2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // Include errno for error reporting
#include <gpiod.h> // Must be included before checking GPIOD_API_VERSION
#include "gpio-fan-rpm.h"

// Read value using libgpiod v2 or v1
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
    if (!chip) {
        fprintf(stderr, "[ERROR] Failed to open chip %s: %s\n", chip_path, strerror(errno));
        return -1;
    }

    int value = -1; // Default to error

#ifdef GPIOD_API_VERSION
    // --- libgpiod v2 Implementation ---
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    struct gpiod_line_request *request = NULL;

    if (!settings || !line_cfg || !req_cfg) {
        fprintf(stderr, "[ERROR-v2] Failed to allocate settings for gpio_get_value\n");
        goto cleanup_v2;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    // Bias setting might be useful here too, add if needed:
    // gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    unsigned int offset = info->gpio_rel;
    if (gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings) < 0) {
        fprintf(stderr, "[ERROR-v2] Failed to add line settings for gpio_get_value\n");
        goto cleanup_v2;
    }

    gpiod_request_config_set_consumer(req_cfg, "gpio-fan-rpm-read");

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request) {
        fprintf(stderr, "[ERROR-v2] Failed to request line %d on %s for gpio_get_value: %s\n", info->gpio_rel, chip_path, strerror(errno));
        goto cleanup_v2;
    }

    // In v2, getting a single value from a multi-line request requires the offset.
    // gpiod_line_request_get_value is deprecated. Use gpiod_line_request_get_value_by_offset.
    // Since we request only one line, offset 0 within the request corresponds to info->gpio_rel.
    enum gpiod_line_value val_enum = gpiod_line_request_get_value_by_offset(request, 0);

    if (val_enum == GPIOD_LINE_VALUE_ERROR) {
         fprintf(stderr, "[ERROR-v2] Failed to get value for line %d on %s: %s\n", info->gpio_rel, chip_path, strerror(errno));
         value = -1;
    } else {
        value = (val_enum == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
    }

cleanup_v2:
    if (request) gpiod_line_request_release(request);
    if (req_cfg) gpiod_request_config_free(req_cfg);
    if (line_cfg) gpiod_line_config_free(line_cfg);
    if (settings) gpiod_line_settings_free(settings);

#else
    // --- libgpiod v1 Implementation ---
    #warning "Compiling gpio_get_value with libgpiod v1 API fallback. GPIO bias settings might not be applied."
    struct gpiod_line *line = gpiod_chip_get_line(chip, info->gpio_rel);
    if (!line) {
        fprintf(stderr, "[ERROR-v1] Failed to get line %d on %s for gpio_get_value: %s\n", info->gpio_rel, chip_path, strerror(errno));
        goto cleanup_v1;
    }

    // Request the line as input
    // Note: v1 bias setting via flags is less flexible than v2 settings.
    if (gpiod_line_request_input_flags(line, "gpio-fan-rpm-read", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        // Fallback to simple input request if pull-up fails or isn't supported
        if (gpiod_line_request_input(line, "gpio-fan-rpm-read") < 0) {
             fprintf(stderr, "[ERROR-v1] Failed to request line %d on %s as input: %s\n", info->gpio_rel, chip_path, strerror(errno));
             // No need to release if request failed
             line = NULL; // Mark line as invalid for cleanup check
             goto cleanup_v1;
        }
    }

    int val_int = gpiod_line_get_value(line);
    if (val_int < 0) {
        fprintf(stderr, "[ERROR-v1] Failed to get value for line %d on %s: %s\n", info->gpio_rel, chip_path, strerror(errno));
        value = -1;
    } else {
        value = val_int; // v1 returns 0 or 1 directly
    }

// Cleanup label for v1 path
cleanup_v1:
    if (line) gpiod_line_release(line); // Release line only if it was successfully acquired/requested

#endif // GPIOD_API_VERSION check

    // Common cleanup for both versions
    if (chip) gpiod_chip_close(chip);

    return value;
}
