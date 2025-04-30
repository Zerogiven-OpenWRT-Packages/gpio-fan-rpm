# Makefile for gpio-fan-rpm

# Compiler and flags
CC=$(CROSS)gcc
CFLAGS+=-Wall -Wextra -pthread
LDFLAGS+=-pthread
LDLIBS=-lgpiod -ljson-c -lpthread

# Use GPIOD_V2=1 for OpenWRT 24.10 (libgpiod v2)
# Use GPIOD_V2=0 or leave unset for OpenWRT 23.05 (libgpiod v1)
ifdef GPIOD_V2
ifeq ($(GPIOD_V2),1)
    # We're compiling for libgpiod v2 (OpenWRT 24.10)
    COMPAT_SRC = gpio_compat_v2.c
    CFLAGS += -DGPIOD_VERSION=2
else
    # We're compiling for libgpiod v1 (OpenWRT 23.05)
    COMPAT_SRC = gpio_compat_v1.c
    CFLAGS += -DGPIOD_VERSION=1
endif
else
    # Default to v1 if not specified
    COMPAT_SRC = gpio_compat_v1.c
    CFLAGS += -DGPIOD_VERSION=1
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
