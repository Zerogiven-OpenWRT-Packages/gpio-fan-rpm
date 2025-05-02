#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <unistd.h>  // for gethostname
#include "format.h"

char* format_numeric(double rpm) {
    char *buf = malloc(32);
    if (!buf) return NULL;
    snprintf(buf, 32, "%.0f\n", rpm);
    return buf;
}

// Unified JSON helper (with GPIO and RPM)
char* format_json(int gpio, double rpm) {
    json_object *j = json_object_new_object();
    json_object_object_add(j, "gpio", json_object_new_int(gpio));
    json_object_object_add(j, "rpm", json_object_new_double(rpm));
    const char *s = json_object_to_json_string_ext(j, JSON_C_TO_STRING_PLAIN);
    size_t len = strlen(s) + 2;
    char *buf = malloc(len);
    if (!buf) { json_object_put(j); return NULL; }
    snprintf(buf, len, "%s\n", s);
    json_object_put(j);
    return buf;
}

char* format_collectd(int gpio, double rpm, int duration) {
    char host[128] = {0};
    gethostname(host, sizeof(host)-1);
    char tmp[256];
    int written = snprintf(tmp, sizeof(tmp),
        "PUTVAL \"%s/gpio-fan-%d/gauge-fan\" interval=%d N:%.0f\n",
        host, gpio, duration, rpm);
    if (written < 0) return NULL;
    char *buf = malloc(written+1);
    if (!buf) return NULL;
    memcpy(buf, tmp, written);
    buf[written] = '\0';
    return buf;
}