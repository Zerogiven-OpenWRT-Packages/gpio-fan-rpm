#ifndef WORKER_H
#define WORKER_H

#include <gpiod.h>
#include <pthread.h>
#include <json-c/json.h>

enum output_mode { MODE_DEFAULT, MODE_NUMERIC, MODE_JSON, MODE_COLLECTD };

// Arguments for each measurement thread
struct thread_args {
    int gpio;
    char *chipname;
    int duration;
    int pulses;
    int debug;
    int watch;
    enum output_mode mode;
};

// Measure RPM on a line
double measure_rpm(struct gpiod_line *line, int pulses_per_rev, int duration, int debug);

// Auto-open gpiochip for given line
struct gpiod_chip *auto_open_chip(int gpio);

// Thread function: performs measurement and prints output
void *thread_fn(void *arg);

// Shared stop flag set by SIGINT
extern volatile int stop;

// Mutex to serialize output
extern pthread_mutex_t print_mutex;

#endif // WORKER_H