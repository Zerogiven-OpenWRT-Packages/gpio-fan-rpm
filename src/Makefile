# Source Makefile for gpio-fan-rpm
# This Makefile is called by the main OpenWRT Makefile

TARGET = gpio-fan-rpm
SOURCES = main.c gpio.c chip.c line.c format.c args.c watch.c measure.c
HEADERS = include/gpio.h include/chip.h include/line.h include/format.h include/args.h include/watch.h include/measure.h

# Add include directory to compiler flags
CFLAGS += -Iinclude

# Default target
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS) $(LIB) -lm

clean:
	rm -f $(TARGET)

.PHONY: all clean 