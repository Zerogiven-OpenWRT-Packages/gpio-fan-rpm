/**
 * @file format.h
 * @brief Output formatting functions for RPM measurements
 * @author CSoellinger
 * @date 2025
 * @license GPL-3.0-only
 * 
 * This module provides functions to format RPM measurements in various
 * output formats including human-readable, JSON, numeric, and collectd.
 */

#ifndef FORMAT_H
#define FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Format RPM as numeric string (with newline)
 * 
 * @param rpm RPM value to format
 * @return char* Formatted string (caller must free), NULL on error
 * 
 * @note Returns a newly allocated string that must be freed by the caller
 */
char* format_numeric(double rpm);

/**
 * @brief Format RPM and GPIO as JSON string (with newline)
 * 
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @return char* Formatted JSON string (caller must free), NULL on error
 * 
 * @note Returns a newly allocated string that must be freed by the caller
 */
char* format_json(int gpio, double rpm);

/**
 * @brief Format multiple GPIO RPM values as JSON array string (with newline)
 * 
 * @param gpios Array of GPIO numbers
 * @param rpms Array of RPM values
 * @param count Number of GPIO/RPM pairs
 * @return char* Formatted JSON array string (caller must free), NULL on error
 * 
 * @note Returns a newly allocated string that must be freed by the caller
 */


/**
 * @brief Format RPM and GPIO as collectd PUTVAL string (with newline)
 * 
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @param duration Measurement duration in seconds
 * @return char* Formatted collectd string (caller must free), NULL on error
 * 
 * @note Returns a newly allocated string that must be freed by the caller
 */
char* format_collectd(int gpio, double rpm, int duration);

/**
 * @brief Format human-readable output
 * 
 * @param gpio GPIO number
 * @param rpm RPM value to format
 * @return char* Formatted string (caller must free), NULL on error
 * 
 * @note Returns a newly allocated string that must be freed by the caller
 */
char* format_human_readable(int gpio, double rpm);

#ifdef __cplusplus
}
#endif

#endif // FORMAT_H 