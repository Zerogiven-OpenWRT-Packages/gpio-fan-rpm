# Makefile for gpio-fan-rpm

# Compiler and flags
CC ?= gcc
CFLAGS ?= -O2
# The order of libraries matters for linking
LIB = -lgpiod -ljson-c -lpthread

# Target executable name
TARGET = gpio-fan-rpm

# Source files
SRCS = gpio-fan-rpm.c args.c gpio.c measure.c gpio-detect.c measure_rpm_edge.c gpiod_compat.c

# Object files
OBJS = $(SRCS:.c=.o)

# Version information from git or a fallback
PKG_TAG ?= 1.0.0-dev
PKG_MAINTAINER ?= CSoellinger
PKG_LICENSE ?= GPL
PKG_COPYRIGHT_YEAR ?= 2025

# Add version information
CFLAGS += -DPKG_TAG=\"$(PKG_TAG)\" \
          -DPKG_MAINTAINER=\"$(PKG_MAINTAINER)\" \
          -DPKG_LICENSE=\"$(PKG_LICENSE)\" \
          -DPKG_COPYRIGHT_YEAR=\"$(PKG_COPYRIGHT_YEAR)\"

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
