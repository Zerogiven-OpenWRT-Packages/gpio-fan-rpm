# Source Makefile for gpio-fan-rpm
# This Makefile is called by the main OpenWRT Makefile

TARGET = gpio-fan-rpm
SOURCES = main.c gpio.c chip.c line.c format.c args.c watch.c measure.c
HEADERS = gpio.h chip.h line.h format.h args.h watch.h measure.h

# Default target
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS) $(LIB) -lm

clean:
	rm -f $(TARGET)

.PHONY: all clean 