/**
 * @file chip.h
 * @brief GPIO chip management operations
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 * 
 * This module handles GPIO chip operations with compatibility for both
 * libgpiod v1 and v2.
 */

#ifndef CHIP_H
#define CHIP_H

#include <gpiod.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Open GPIO chip by name
 * 
 * @param name Chip name (e.g., "gpiochip0")
 * @return struct gpiod_chip* Chip object or NULL on error
 */
struct gpiod_chip* chip_open_by_name(const char *name);

/**
 * @brief Close GPIO chip
 * 
 * @param chip Chip to close
 */
void chip_close(struct gpiod_chip *chip);

/**
 * @brief Auto-detect available GPIO chip for given line
 * 
 * @param gpio GPIO line number
 * @param chipname_out Output parameter for chip name (caller must free)
 * @return struct gpiod_chip* Chip object or NULL on error
 */
struct gpiod_chip* chip_auto_detect(int gpio, char **chipname_out);

/**
 * @brief Get chip information
 * 
 * @param chip Chip object
 * @return struct gpiod_chip_info* Chip info or NULL on error
 */
struct gpiod_chip_info* chip_get_info(struct gpiod_chip *chip);

/**
 * @brief Get number of lines on chip
 * 
 * @param chip Chip object
 * @return size_t Number of lines, 0 on error
 */
size_t chip_get_num_lines(struct gpiod_chip *chip);

/**
 * @brief Get chip name from chip info
 * 
 * @param chip Chip object
 * @return const char* Chip name or NULL on error
 */
const char* chip_get_name(struct gpiod_chip *chip);

/**
 * @brief Get chip label from chip info
 * 
 * @param chip Chip object
 * @return const char* Chip label or NULL on error
 */
const char* chip_get_label(struct gpiod_chip *chip);

#ifdef __cplusplus
}
#endif

#endif // CHIP_H 