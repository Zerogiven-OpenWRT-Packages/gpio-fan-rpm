#!/bin/sh

# Compile test_format
CC=${CC:-gcc}
CFLAGS="-I../src -Wall -Wextra"
LIBS="-ljson-c -pthread"

$CC $CFLAGS test_format.c -o test_format $LIBS

# Run tests
./test_format