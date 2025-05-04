#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include "worker.h"
#include "format.h"

#ifndef PKG_TAG
#define PKG_TAG_STR "unknown"
#else
#define _STR(x) #x
#define STR(x) _STR(x)
#define PKG_TAG_STR STR(PKG_TAG)
#endif

static void usage(const char *prog) {
    printf("Usage: %s [options]\n"
           "Options:\n"
           "  --gpio=N, -g N         GPIO number to measure (can be repeated)\n"
           "  --chip=NAME, -c NAME   GPIO chip (default: auto-detect per GPIO)\n"
           "  --duration=SEC, -d SEC Duration to measure (default: 2)\n"
           "  --pulses=N, -p N       Pulses per revolution (default: 2)\n"
           "  --numeric              Output RPM as numeric only\n"
           "  --json                 Output as JSON\n"
           "  --collectd             Output in collectd PUTVAL format\n"
           "  --debug                Show debug output\n"
           "  --watch, -w            Repeat measurements continuously\n"
           "  --help, -h             Show this help\n"
           "  --version, -v          Show version information\n"
           "\n"
           "Note:\n"
           "  The --duration value is internally split into two phases:\n"
           "    • Warmup phase:      duration / 2 (no output)\n"
           "    • Measurement phase: duration / 2 (output shown)\n"
           "  In --watch mode, the warmup is only performed once (before the loop).\n"
           "  Each loop iteration then only uses duration / 2 for measurement.\n",
           prog);
}

// SIGINT handler: set stop flag for worker threads
static void sigint_handler(int sig) {
    (void)sig;
    stop = 1;
}

int main(int argc, char **argv) {
    int duration = 2, pulses = 2;
    char *chipname = NULL;
    int debug = 0, watch = 0;
    int mode = MODE_DEFAULT;
    int *gpios = NULL;
    size_t ngpio = 0;
    int opt;
    struct option longopts[] = {
        {"gpio", required_argument, 0, 'g'},
        {"chip", required_argument, 0, 'c'},
        {"duration", required_argument, 0, 'd'},
        {"pulses", required_argument, 0, 'p'},
        {"numeric", no_argument, &mode, MODE_NUMERIC},
        {"json", no_argument, &mode, MODE_JSON},
        {"collectd", no_argument, &mode, MODE_COLLECTD},
        {"debug", no_argument, 0, 'D'},
        {"watch", no_argument, 0, 'w'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0,0,0,0}
    };
    while ((opt = getopt_long(argc, argv, "g:c:d:p:Dwhv", longopts, NULL)) != -1) {
        switch (opt) {
        case 'g':
            gpios = realloc(gpios, (ngpio+1)*sizeof(*gpios));
            gpios[ngpio++] = atoi(optarg);
            break;
        case 'c': chipname = strdup(optarg); break;
        case 'd': duration = atoi(optarg); break;
        case 'p': pulses = atoi(optarg); break;
        case 'D': debug = 1; break;
        case 'w': watch = 1; break;
        case 'h': usage(argv[0]); return 0;
        case 'v': printf("%s %s\n", argv[0], PKG_TAG_STR); return 0;
        case 0: break;
        default: usage(argv[0]); return 1;
        }
    }
    if (ngpio == 0) {
        fprintf(stderr, "Error: at least one --gpio required\n");
        usage(argv[0]);
        return 1;
    }
    signal(SIGINT, sigint_handler);

    // Spawn a thread per GPIO
    pthread_t *threads = calloc(ngpio, sizeof(*threads));
    for (size_t i = 0; i < ngpio; i++) {
        struct thread_args *a = calloc(1, sizeof(*a));
        a->gpio = gpios[i];
        a->chipname = chipname;  // will be freed at exit
        a->duration = duration;
        a->pulses = pulses;
        a->debug = debug;
        a->watch = watch;
        a->mode = mode;
        int ret = pthread_create(&threads[i], NULL, thread_fn, a);
        if (ret) {
            fprintf(stderr, "Error: cannot create thread for GPIO %d: %s\n", a->gpio, strerror(ret));
            free(a);
            threads[i] = 0;
        }
    }
    // Wait for threads to finish
    for (size_t i = 0; i < ngpio; i++) {
        if (threads[i]) pthread_join(threads[i], NULL);
    }
    free(threads);
    free(gpios);
    if (chipname) free(chipname);
    return 0;
}