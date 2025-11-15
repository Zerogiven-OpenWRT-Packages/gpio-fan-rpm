/**
 * @file chip.c
 * @brief GPIO chip management operations implementation
 * @author CSoellinger
 * @date 2025
 * @license LGPL-3.0-or-later
 * 
 * This module implements GPIO chip operations with compatibility for both
 * libgpiod v1 and v2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip.h"

struct gpiod_chip* chip_open_by_name(const char *name) {
    if (!name) return NULL;

#ifdef LIBGPIOD_V2
    // For libgpiod v2, construct the device path
    char path[64];
    int n = snprintf(path, sizeof(path), "/dev/%s", name);
    if (n < 0 || n >= (int)sizeof(path)) {
        return NULL; // Path truncated or error
    }
    return gpiod_chip_open(path);
#else
    return gpiod_chip_open_by_name(name);
#endif
}

void chip_close(struct gpiod_chip *chip) {
    if (chip) {
        gpiod_chip_close(chip);
    }
}

struct gpiod_chip* chip_auto_detect(int gpio, char **chipname_out) {
    if (!chipname_out) return NULL;
    *chipname_out = NULL;
    
    char name[16];
    
    // Try gpiochip0 through gpiochip9
    for (int i = 0; i < 10; i++) {
        int n = snprintf(name, sizeof(name), "gpiochip%d", i);
        if (n < 0 || n >= (int)sizeof(name)) {
            continue; // Skip if name was truncated
        }

        struct gpiod_chip *chip = chip_open_by_name(name);
        if (!chip) continue;
        
        // Check if this chip has enough lines
#ifdef LIBGPIOD_V2
        struct gpiod_chip_info *info = chip_get_info(chip);
        if (info) {
            size_t num_lines = gpiod_chip_info_get_num_lines(info);
            gpiod_chip_info_free(info);

            if (gpio < (int)num_lines) {
                *chipname_out = strdup(name);
                if (!*chipname_out) {
                    chip_close(chip);
                    return NULL;
                }
                return chip;
            }
        }
#else
        // In libgpiod v1, try to get a line to check if it exists
        struct gpiod_line *line = gpiod_chip_get_line(chip, gpio);
        if (line) {
            gpiod_line_release(line);
            *chipname_out = strdup(name);
            if (!*chipname_out) {
                chip_close(chip);
                return NULL;
            }
            return chip;
        }
#endif
        
        chip_close(chip);
    }
    
    return NULL;
}

int chip_auto_detect_for_name(int gpio, char **chipname_out) {
    if (!chipname_out) return -1;

    struct gpiod_chip *chip = chip_auto_detect(gpio, chipname_out);
    if (chip) {
        chip_close(chip);
        return 0;
    }

    return -1;
}

struct gpiod_chip_info* chip_get_info(struct gpiod_chip *chip) {
    if (!chip) return NULL;
#ifdef LIBGPIOD_V2
    return gpiod_chip_get_info(chip);
#else
    // In libgpiod v1, chip_info concept doesn't exist
    // Return NULL to indicate this function is not available
    return NULL;
#endif
}

size_t chip_get_num_lines(struct gpiod_chip *chip) {
    if (!chip) return 0;
    
#ifdef LIBGPIOD_V2
    struct gpiod_chip_info *info = chip_get_info(chip);
    if (!info) return 0;
    
    size_t num_lines = gpiod_chip_info_get_num_lines(info);
    gpiod_chip_info_free(info);
    
    return num_lines;
#else
    // In libgpiod v1, we can't easily get the total number of lines
    // Return a reasonable default or try to estimate
    // For now, return a high number to be safe
    return 64; // Most GPIO chips have fewer than 64 lines
#endif
}

/**
 * @brief Get chip name from chip info
 *
 * NOTE: Currently unused but reserved for future debugging/informational features.
 *
 * @param chip Chip object
 * @return char* Allocated string with chip name or NULL on error. Caller must free().
 */
__attribute__((unused)) char* chip_get_name(struct gpiod_chip *chip) {
    if (!chip) return NULL;

#ifdef LIBGPIOD_V2
    struct gpiod_chip_info *info = chip_get_info(chip);
    if (!info) return NULL;

    const char *name = gpiod_chip_info_get_name(info);
    char *result = name ? strdup(name) : NULL;
    gpiod_chip_info_free(info);

    return result;
#else
    // In libgpiod v1, get name directly from chip and duplicate it
    const char *name = gpiod_chip_name(chip);
    return name ? strdup(name) : NULL;
#endif
}

/**
 * @brief Get chip label from chip info
 *
 * NOTE: Currently unused but reserved for future debugging/informational features.
 *
 * @param chip Chip object
 * @return char* Allocated string with chip label or NULL on error. Caller must free().
 */
__attribute__((unused)) char* chip_get_label(struct gpiod_chip *chip) {
    if (!chip) return NULL;

#ifdef LIBGPIOD_V2
    struct gpiod_chip_info *info = chip_get_info(chip);
    if (!info) return NULL;

    const char *label = gpiod_chip_info_get_label(info);
    char *result = label ? strdup(label) : NULL;
    gpiod_chip_info_free(info);

    return result;
#else
    // In libgpiod v1, get label directly from chip and duplicate it
    const char *label = gpiod_chip_label(chip);
    return label ? strdup(label) : NULL;
#endif
} 