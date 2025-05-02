#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include "format.h"

char* format_numeric(double rpm) {
    char *buf = malloc(32);
    if (!buf) return NULL;
    snprintf(buf, 32, "%.0f\n", rpm);
    return buf;
}

char* format_json(double rpm) {
    json_object *j = json_object_new_object();
    json_object_object_add(j, "rpm", json_object_new_double(rpm));
    const char *s = json_object_to_json_string_ext(j, JSON_C_TO_STRING_PLAIN);
    size_t len = strlen(s) + 2;
    char *buf = malloc(len);
    if (!buf) { json_object_put(j); return NULL; }
    snprintf(buf, len, "%s\n", s);
    json_object_put(j);
    return buf;
}