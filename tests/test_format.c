#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/format.h"

int main(void) {
    // Numeric formatting
    char *num = format_numeric(1234.56);
    assert(num != NULL);
    assert(strcmp(num, "1235\n") == 0);
    free(num);

    // JSON formatting with GPIO identifier
    int test_gpio = 3;
    char *j = format_json(test_gpio, 12.34);
    assert(j != NULL);
    assert(strcmp(j, "{\"gpio\":3,\"rpm\":12.34}\n") == 0);
    free(j);

    printf("All format tests passed.\n");
    return 0;
}