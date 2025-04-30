// measure.c
// Uses libgpiod v2 edge events to measure RPMs in parallel

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>

#include "gpio-fan-rpm.h"

// Perform measurements and optionally print output
void perform_measurements(config_t *cfg, int skip_output)
{
    if (cfg->gpio_count <= 0)
    {
        fprintf(stderr, "No GPIOs specified for measurement\n");
        return;
    }

    // Find available GPIO chips if not specified
    if (cfg->default_chip[0] == '\0')
        detect_chip(cfg);

    // Perform measurements for each configured GPIO
    for (int i = 0; i < cfg->gpio_count; i++)
    {
        gpio_info_t *info = &cfg->gpios[i];
        
        // If no chip specified for this GPIO, use the default
        if (info->chip[0] == '\0' && cfg->default_chip[0] != '\0')
            strncpy(info->chip, cfg->default_chip, sizeof(info->chip) - 1);
            
        if (cfg->debug)
            fprintf(stderr, "[DEBUG] Measuring GPIO %d on %s\n", info->gpio_rel, info->chip);
            
        // Use the edge detection method to measure RPM
        float rpm = measure_rpm_edge(info->chip, info->gpio_rel, cfg->debug);
        
        if (rpm < 0)
        {
            // Measurement failed
            info->valid = 0;
            info->rpm = 0;
            if (cfg->debug)
                fprintf(stderr, "[DEBUG] Failed to measure RPM for GPIO %d on %s\n", 
                        info->gpio_rel, info->chip);
        }
        else
        {
            // Store the measured RPM
            info->valid = 1;
            info->rpm = (int)(rpm + 0.5f); // Round to nearest integer
            
            if (cfg->debug)
                fprintf(stderr, "[DEBUG] Measured RPM: %d for GPIO %d on %s\n", 
                        info->rpm, info->gpio_rel, info->chip);
        }
    }

    // Skip output if requested
    if (skip_output)
        return;

    if (cfg->json_output)
    {
        json_object *array = json_object_new_array();

        for (int i = 0; i < cfg->gpio_count; i++)
        {
            gpio_info_t *g = &cfg->gpios[i];
            if (!g->valid)
                continue;

            json_object *obj = json_object_new_object();
            json_object_object_add(obj, "gpio", json_object_new_int(g->gpio_rel));
            json_object_object_add(obj, "rpm", json_object_new_int(g->rpm));
            json_object_array_add(array, obj);
        }

        printf("%s\n", json_object_to_json_string_ext(array, JSON_C_TO_STRING_PLAIN));
        json_object_put(array);
        return;
    }

    for (int i = 0; i < cfg->gpio_count; i++)
    {
        gpio_info_t *g = &cfg->gpios[i];

        if (!g->valid)
        {
            if (!cfg->numeric_output && !cfg->collectd_output)
                fprintf(stderr, "GPIO %d: read error\n", g->gpio_rel);
            continue;
        }

        if (cfg->numeric_output)
        {
            printf("%d\n", g->rpm);
        }
        else if (cfg->collectd_output)
        {
            printf("PUTVAL \"openwrt/gpio-fan-rpm/gpio-%d-rpm\" interval=%d N:%d\n",
                   g->gpio_rel, cfg->duration, g->rpm);
        }
        else
        {
            printf("GPIO %d: %d RPM\n", g->gpio_rel, g->rpm);
        }
    }
}
