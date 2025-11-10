/**
 * @file chip.h
 * @brief GPIO chip management operations
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 *
 * This module handles GPIO chip operations with compatibility for both
 * libgpiod v1 and v2.
 *
 * @section return_values Return Value Conventions
 *
 * This project follows consistent return value patterns:
 *
 * - **int functions**: Return 0 on success, -1 on error
 *   - Exception: parse_arguments() returns 1 for help/version
 *   - Wait functions: 0=timeout, 1=event ready, -1=error
 *
 * - **Pointer functions**: Return valid pointer on success, NULL on error
 *   - Allocated strings must be freed by caller (documented with @note)
 *
 * - **size_t functions**: Return count on success, 0 on error
 *
 * - **double functions**: Return measured value, -1.0 on interrupt, 0.0 if no data
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
 * @brief Auto-detect GPIO chip and immediately close it
 *
 * Convenience function that auto-detects the chip and closes it immediately.
 * Useful when you only need the chip name, not the chip handle.
 *
 * @param gpio GPIO line number
 * @param chipname_out Output parameter for chip name (caller must free)
 * @return int 0 on success, -1 on error
 */
int chip_auto_detect_for_name(int gpio, char **chipname_out);

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
 * @return char* Allocated string with chip name or NULL on error. Caller must free().
 */
char* chip_get_name(struct gpiod_chip *chip);

/**
 * @brief Get chip label from chip info
 *
 * @param chip Chip object
 * @return char* Allocated string with chip label or NULL on error. Caller must free().
 */
char* chip_get_label(struct gpiod_chip *chip);

#ifdef __cplusplus
}
#endif

#endif // CHIP_H 