#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>

enum output_mode { MODE_DEFAULT, MODE_NUMERIC, MODE_JSON, MODE_COLLECTD };

struct thread_args {
    int gpio;
    char *chipname;
    int duration;
    int pulses;
    int debug;
    int watch;
    int mode;
};

// Thread entry: measurement per GPIO
void *thread_fn(void *arg);

// Globals
extern volatile int stop;
extern pthread_mutex_t print_mutex;

#endif // WORKER_H