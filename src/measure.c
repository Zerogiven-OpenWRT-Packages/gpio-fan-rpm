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
    if (cfg->debug)
    {
        printf("[DEBUG] Starting edge-based measurement on %d GPIO(s)\n", cfg->gpio_count);
        printf("[DEBUG] Duration: %d second(s)\n", cfg->duration);
        printf("[DEBUG] Global pulses/rev: %d\n", cfg->pulses_per_rev);
    }

    // Apply pulses_per_rev to all gpio_info structs (if unset)
    for (int i = 0; i < cfg->gpio_count; i++)
    {
        if (cfg->gpios[i].pulses_per_rev <= 0)
            cfg->gpios[i].pulses_per_rev = cfg->pulses_per_rev;

        if (cfg->debug)
        {
            printf("[DEBUG] GPIO %d → chip=%s, pulses/rev=%d\n",
                   cfg->gpios[i].gpio_rel,
                   cfg->gpios[i].chip,
                   cfg->gpios[i].pulses_per_rev);
        }
    }

    if (measure_rpm_edge(cfg->gpios, cfg->gpio_count, cfg->duration, cfg->debug) < 0)
    {
        fprintf(stderr, "Error: failed to measure RPM using edge events\n");
        return;
    }

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
