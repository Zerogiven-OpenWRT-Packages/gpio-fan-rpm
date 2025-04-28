# Makefile for gpio-fan-rpm (modular build)

# Compiler and flags
CC ?= gcc
CFLAGS ?= -O2
# The order of libraries matters for linking
LIB = -lgpiod -ljson-c -lpthread

# Target executable name
TARGET = gpio-fan-rpm

# Source files (modular)
SRCS = gpio-fan-rpm.c args.c gpio.c measure.c gpio-detect.c measure_rpm_edge.c

# Object files
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
