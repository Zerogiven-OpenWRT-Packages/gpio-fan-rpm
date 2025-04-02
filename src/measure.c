// measure.c
// Measures RPM via GPIO polling and prints output in various formats

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <errno.h>
#include "gpio-fan-rpm.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 128

// Helper function for precise time measurement
static inline long long current_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Debouncing logic
static int debounce_signal(int gpio_abs) {
    static const int debounce_delay_ns = 5000000; // 5ms
    static long long last_signal_time = 0;

    long long now = current_time_ns();
    if (now - last_signal_time < debounce_delay_ns) {
        return 0; // Ignore signal
    }
    last_signal_time = now;
    return 1; // Valid signal
}

// Measure RPM from GPIO using poll() for falling edges
int measure_rpm(int gpio_abs, int duration_sec, int pulses_per_rev)
{
    char path[MAX_BUF];
    snprintf(path, sizeof(path), SYSFS_GPIO_DIR "/gpio%d/value", gpio_abs);
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        if (errno == ENOENT)
            fprintf(stderr, "[ERROR] GPIO %d not exported or not available: %s\n", gpio_abs, path);
        else
            perror("open");
        return -1;
    }

    struct pollfd pfd = {.fd = fd, .events = POLLPRI | POLLERR};
    char buf[MAX_BUF];
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, sizeof(buf));

    int count = 0;
    time_t start = time(NULL);
    while (time(NULL) - start < duration_sec)
    {
        int ret = poll(&pfd, 1, 1000);
        if (ret > 0)
        {
            lseek(fd, 0, SEEK_SET);
            read(fd, buf, sizeof(buf));
            count++;
        }
    }
    close(fd);
    return (count * 60) / pulses_per_rev / duration_sec;
}

// Perform measurements and print output
void perform_measurements(config_t *cfg)
{
    for (int i = 0; i < cfg->gpio_count; i++)
    {
        gpio_info_t *g = &cfg->gpios[i];

        // Skip if gpio_rel is invalid
        if (g->gpio_rel < 0)
        {
            g->valid = 0;
            if (cfg->debug)
                fprintf(stderr, "[DEBUG] Invalid gpio_rel (not set) at index %d\n", i);
            continue;
        }

        // Fallback: use default base if base not set
        if (g->base <= 0 && cfg->default_base > 0)
        {
            g->base = cfg->default_base;
        }

        // Try to auto-detect base from chip
        if (g->base <= 0)
        {
            g->base = read_gpio_base(g->chip);
        }

        if (cfg->debug)
            printf("[DEBUG] GPIO %d using chip %s base %d\n", g->gpio_rel, g->chip, g->base);

        g->gpio_abs = g->base + g->gpio_rel;

        if (cfg->debug)
            printf("[DEBUG] Exporting GPIO %d (abs %d)\n", g->gpio_rel, g->gpio_abs);

        if (export_gpio(g->gpio_abs) < 0)
        {
            g->valid = 0;
            if (!cfg->numeric_output && !cfg->json_output && !cfg->collectd_output)
                fprintf(stderr, "GPIO %d: export failed\n", g->gpio_rel);
            continue;
        }

        setup_gpio(g->gpio_abs);

        if (!cfg->numeric_output && !cfg->json_output && !cfg->collectd_output)
            printf("Measuring GPIO %d for %d second(s)...\n", g->gpio_rel, cfg->duration);

        int pulse_count = 0;
        long long start_time = current_time_ns();

        while ((current_time_ns() - start_time) < cfg->duration * 1e9) {
            if (read_gpio(g->gpio_abs) && debounce_signal(g->gpio_abs)) {
                pulse_count++;
            }
        }

        // Calculate RPM and filter outliers
        int rpm = (pulse_count * 60) / (cfg->pulses_per_rev * cfg->duration);
        if (rpm > 0 && rpm < 10000) { // Example filter: plausible RPM range
            g->rpm = rpm;
        } else {
            g->rpm = -1; // Mark as invalid
        }

        if (cfg->debug) {
            fprintf(stderr, "[DEBUG] GPIO %d: pulses=%d, rpm=%d\n", g->gpio_rel, pulse_count, g->rpm);
        }

        g->valid = 1;
    }

    for (int i = 0; i < cfg->gpio_count; i++)
    {
        gpio_info_t *g = &cfg->gpios[i];
        if (!g->valid)
            continue;

        if (cfg->numeric_output)
            printf("%d\n", g->rpm);
        else if (cfg->json_output)
            printf("{\"gpio\":%d,\"rpm\":%d}\n", g->gpio_rel, g->rpm);
        else if (cfg->collectd_output)
            printf("PUTVAL \"openwrt/gpio-fan-rpm/gpio-%d-rpm\" interval=%d N:%d\n",
                   g->gpio_rel, cfg->duration, g->rpm);
        else
            printf("GPIO %d: %d RPM\n", g->gpio_rel, g->rpm);
    }
}
