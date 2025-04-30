# Makefile for gpio-fan-rpm

# Compiler and flags
CC=$(CROSS)gcc
CFLAGS+=-Wall -Wextra -pthread
LDFLAGS+=-pthread
LDLIBS=-lgpiod -ljson-c -lpthread

# Sources and objects
SOURCES=gpio-fan-rpm.c args.c gpio.c measure.c gpio-detect.c measure_rpm_edge.c
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
