/**
 * @file chip.c
 * @brief GPIO chip management operations implementation
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
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
    snprintf(path, sizeof(path), "/dev/%s", name);
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
        snprintf(name, sizeof(name), "gpiochip%d", i);
        
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
                return chip;
            }
        }
#else
        // In libgpiod v1, try to get a line to check if it exists
        struct gpiod_line *line = gpiod_chip_get_line(chip, gpio);
        if (line) {
            gpiod_line_release(line);
            *chipname_out = strdup(name);
            return chip;
        }
#endif
        
        chip_close(chip);
    }
    
    return NULL;
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
 * @param chip Chip object
 * @return const char* Chip name or NULL on error
 */
const char* chip_get_name(struct gpiod_chip *chip) {
    if (!chip) return NULL;
    
#ifdef LIBGPIOD_V2
    struct gpiod_chip_info *info = chip_get_info(chip);
    if (!info) return NULL;
    
    const char *name = gpiod_chip_info_get_name(info);
    gpiod_chip_info_free(info);
    
    return name;
#else
    // In libgpiod v1, get name directly from chip
    return gpiod_chip_name(chip);
#endif
}

/**
 * @brief Get chip label from chip info
 * 
 * @param chip Chip object
 * @return const char* Chip label or NULL on error
 */
const char* chip_get_label(struct gpiod_chip *chip) {
    if (!chip) return NULL;
    
#ifdef LIBGPIOD_V2
    struct gpiod_chip_info *info = chip_get_info(chip);
    if (!info) return NULL;
    
    const char *label = gpiod_chip_info_get_label(info);
    gpiod_chip_info_free(info);
    
    return label;
#else
    // In libgpiod v1, get label directly from chip
    return gpiod_chip_label(chip);
#endif
} 