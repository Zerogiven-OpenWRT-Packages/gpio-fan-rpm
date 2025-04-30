# Makefile for gpio-fan-rpm

# Compiler and flags
CC=$(CROSS)gcc
CFLAGS+=-Wall -Wextra -pthread
LDFLAGS+=-pthread
LDLIBS=-lgpiod -ljson-c -lpthread

# Detect libgpiod version to choose the right compat implementation
VERSION_TEST := $(shell $(CC) -E -dM -xc /dev/null | grep -q GPIOD_LINE_DIRECTION_INPUT || echo v1)
ifeq ($(VERSION_TEST), v1)
    # We're compiling for libgpiod v1 (OpenWRT 23.05)
    COMPAT_SRC = gpio_compat_v1.c
else
    # We're compiling for libgpiod v2 (OpenWRT 24.10)
    COMPAT_SRC = gpio_compat_v2.c
endif

# Sources and objects
SOURCES=gpio-fan-rpm.c args.c gpio.c measure.c gpio-detect.c measure_rpm_edge.c $(COMPAT_SRC)
OBJECTS=$(SOURCES:.c=.o)
TARGET=gpio-fan-rpm

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
