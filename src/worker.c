#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <json-c/json.h>
#include <gpiod.h>
#include <pthread.h>
#include "worker.h"
#include "format.h"

// Workaround for missing prototypes in some libgpiod headers
extern int gpiod_line_event_wait(struct gpiod_line *line, struct timespec *timeout);
extern int gpiod_line_event_read(struct gpiod_line *line, struct gpiod_line_event *event);
extern struct gpiod_chip *gpiod_chip_open_by_name(const char *name);
extern struct gpiod_line *gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset);
extern void gpiod_chip_close(struct gpiod_chip *chip);
#ifdef GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES
// v2 request API already in gpiod.h
#else
extern int gpiod_line_request_both_edges_events(struct gpiod_line *line, const char *consumer);
#endif

// Shared stop flag set by SIGINT
volatile int stop = 0;

// Mutex to serialize output
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Detect libgpiod v2 vs v1
#ifdef GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES
#define REQUEST_BOTH_EDGES(line, consumer) gpiod_line_request(line, GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, consumer)
#else
#define REQUEST_BOTH_EDGES(line, consumer) gpiod_line_request_both_edges_events(line, consumer)
#endif

// Measure RPM on a line (internal)
static double measure_rpm(struct gpiod_line *line, int pulses_per_rev, int duration, int debug) {
    struct timespec start_ts, ev_ts;
    unsigned int count = 0;
    int half = duration / 2;

    // Warmup phase
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    while (1) {
        if (gpiod_line_event_wait(line, &ev_ts) < 0) break;
        gpiod_line_event_read(line, NULL);
        clock_gettime(CLOCK_MONOTONIC, &ev_ts);
        if ((ev_ts.tv_sec - start_ts.tv_sec) >= half) break;
    }

    // Measurement phase
    count = 0;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    while (!stop) {
        if (gpiod_line_event_wait(line, &ev_ts) < 0) break;
        gpiod_line_event_read(line, NULL);
        count++;
        clock_gettime(CLOCK_MONOTONIC, &ev_ts);
        if ((ev_ts.tv_sec - start_ts.tv_sec) >= half) break;
    }
    double elapsed = (ev_ts.tv_sec - start_ts.tv_sec) + (ev_ts.tv_nsec - start_ts.tv_nsec)/1e9;

    double revs = (double)count / pulses_per_rev;
    double rpm = revs / elapsed * 60.0;
    if (debug) fprintf(stderr, "Counted %u pulses in %.3f s, RPM=%.1f\n", count, elapsed, rpm);
    return rpm;
}

// Auto-open gpiochip for given line (tries gpiochip0..9)
struct gpiod_chip *auto_open_chip(int gpio) {
    char name[16];
    struct gpiod_chip *c;
    for (int i = 0; i < 10; i++) {
        snprintf(name, sizeof(name), "gpiochip%d", i);
        c = gpiod_chip_open_by_name(name);
        if (!c) continue;
        if (gpiod_chip_get_line(c, gpio)) return c;
        gpiod_chip_close(c);
    }
    return NULL;
}

// Thread entry: opens line, performs warmup/measurement, prints results
void *thread_fn(void *arg) {
    struct thread_args *a = arg;
    struct gpiod_chip *chip;
    if (a->chipname) {
        chip = gpiod_chip_open_by_name(a->chipname);
    } else {
        chip = auto_open_chip(a->gpio);
    }
    if (!chip) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Error: cannot open chip for GPIO %d\n", a->gpio);
        pthread_mutex_unlock(&print_mutex);
        free(a);
        return NULL;
    }
    struct gpiod_line *line = gpiod_chip_get_line(chip, a->gpio);
    if (!line) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Error: cannot get line %d\n", a->gpio);
        pthread_mutex_unlock(&print_mutex);
        gpiod_chip_close(chip);
        free(a);
        return NULL;
    }
    if (REQUEST_BOTH_EDGES(line, "gpio-fan-rpm") < 0) {
        pthread_mutex_lock(&print_mutex);
        perror("gpiod_line_request events");
        pthread_mutex_unlock(&print_mutex);
        gpiod_chip_close(chip);
        free(a);
        return NULL;
    }
    // Warmup once for watch mode
    if (a->watch) measure_rpm(line, a->pulses, a->duration, a->debug);

    // Measurement loop
    do {
        double rpm = measure_rpm(line, a->pulses, a->duration, a->debug);
        pthread_mutex_lock(&print_mutex);
        switch (a->mode) {
        case MODE_NUMERIC: {
            char *s = format_numeric(rpm);
            printf("GPIO%u: %s", a->gpio, s);
            free(s);
            break;
        }
        case MODE_JSON: {
            char *s = format_json(a->gpio, rpm);
            printf("%s", s);
            free(s);
            break;
        }
        case MODE_COLLECTD: {
            char *s = format_collectd(a->gpio, rpm, a->duration);
            printf("%s", s);
            free(s);
            break;
        }
        default:
            printf("GPIO%u: RPM: %.0f\n", a->gpio, rpm);
            break;
        }
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
    } while (a->watch && !stop);

    gpiod_line_release(line);
    gpiod_chip_close(chip);
    free(a);
    return NULL;
}