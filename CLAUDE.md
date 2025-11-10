# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

gpio-fan-rpm is a C-based command-line utility for measuring fan RPM on OpenWRT devices using GPIO edge detection. The project must maintain compatibility with both libgpiod v1 (OpenWRT 23.05) and libgpiod v2 (OpenWRT 24.10).

## Build System

### Building the Package

```bash
# Build for OpenWRT (from OpenWRT build root)
make package/gpio-fan-rpm/compile V=s

# Clean build
make package/gpio-fan-rpm/clean
make package/gpio-fan-rpm/compile V=s
```

### Local Development Build

The src/Makefile is designed to be called by the main OpenWRT Makefile. For local testing, you would need to set up the appropriate cross-compilation environment with the required libraries.

### Testing

There is no automated test suite. Testing requires:
1. OpenWRT device with GPIO pins
2. Fan with tachometer output connected to GPIO
3. Manual verification of RPM readings

```bash
# Basic functionality test
gpio-fan-rpm --gpio=17 --debug

# Multi-GPIO test
gpio-fan-rpm --gpio=17 --gpio=18 --debug

# Watch mode test
gpio-fan-rpm --gpio=17 --watch --duration=4
```

## Project Structure

```
gpio-fan-rpm/
├── src/
│   ├── include/          # Header files
│   │   ├── args.h
│   │   ├── chip.h
│   │   ├── format.h
│   │   ├── gpio.h
│   │   ├── line.h
│   │   ├── measure.h
│   │   └── watch.h
│   ├── main.c           # Entry point
│   ├── gpio.c           # GPIO operations
│   ├── chip.c           # Chip management
│   ├── line.c           # Line operations
│   ├── args.c           # Argument parsing
│   ├── format.c         # Output formatting
│   ├── measure.c        # Single measurement mode
│   ├── watch.c          # Watch mode
│   └── Makefile         # Source build file
├── files/
│   └── etc/config/      # UCI configuration
├── Makefile             # OpenWRT package Makefile
└── README.md
```

## Architecture

### libgpiod Version Compatibility Layer

The codebase uses conditional compilation to support both libgpiod v1 and v2:

- **Version detection**: Makefile detects libgpiod version at compile time by checking for v2-specific symbols in headers
- **Compile flags**: Sets either `-DLIBGPIOD_V1` or `-DLIBGPIOD_V2`
- **Abstraction modules**: `chip.c` and `line.c` provide unified APIs that branch internally based on version

Key differences between versions:
- **v1**: Uses `gpiod_line` objects, direct line operations
- **v2**: Uses `gpiod_line_request` objects with configuration structures, edge event buffers

### Module Structure

**Core modules** (contain version-specific code):
- `chip.c/h`: GPIO chip discovery and management - handles opening chips and auto-detection
- `line.c/h`: GPIO line operations - handles event request setup with edge detection configuration
- `gpio.c/h`: High-level GPIO operations - manages edge event reading, RPM measurement algorithm, thread management

**Application modules** (version-agnostic):
- `main.c`: Entry point, signal handling, orchestration
- `args.c/h`: Command-line parsing with UCI configuration integration
- `measure.c/h`: Single measurement mode with parallel GPIO support
- `watch.c/h`: Continuous monitoring mode
- `format.c/h`: Output formatting (human-readable, JSON, numeric, collectd)

### Measurement Algorithm

Located in `gpio.c:gpio_measure_rpm()`:

1. **Warmup phase**: 1 second (fixed) for fan stabilization - events are read and discarded
2. **Measurement phase**: (duration - 1) seconds - counts edge events (both rising and falling)
3. **RPM calculation**: `rpm = (event_count / pulses_per_rev) / elapsed_time * 60`

The algorithm uses `CLOCK_MONOTONIC` for timing and supports graceful interruption via the global `stop` flag.

### Thread Management

For multiple GPIO pins, each pin runs in its own thread (`gpio_thread_fn`):
- Threads synchronize using `results_mutex` and `all_finished` condition variable
- Results stored in shared `results[]` array indexed by thread
- Main thread waits for all threads to complete, then outputs results in order
- Watch mode runs continuously per-thread with synchronized output via `print_mutex`

### UCI Configuration

OpenWRT's UCI system provides default values:
- Config file: `files/etc/config/gpio-fan-rpm`
- Supports setting default `duration` and `pulses` values
- Parsed in `args.c` via libuci

## Important Implementation Constraints

### libgpiod Version Handling

When modifying GPIO operations:
1. Always check if changes affect both v1 and v2 code paths
2. Test compilation with both `-DLIBGPIOD_V1` and `-DLIBGPIOD_V2`
3. Keep abstractions in `chip.c` and `line.c` - avoid leaking version-specific APIs

### Thread Safety

- Use `print_mutex` for any console output in threaded contexts
- Use `results_mutex` when accessing shared `results[]` or `finished[]` arrays
- The global `stop` flag is volatile and read-only in worker threads

### Memory Management

- All malloc'd memory must have corresponding free (especially in error paths)
- Thread args are freed by the thread function, not the caller
- Chipname strings are strdup'd and must be freed by caller

### Signal Handling

- SIGINT and SIGTERM set the global `stop` flag
- Threads check `stop` during measurement loops for graceful shutdown
- Interrupted measurements return -1.0 and are filtered from output

## Common Development Tasks

### Adding a New Output Format

1. Add enum value to `output_mode_t` in `gpio.h`
2. Implement formatter function in `format.c` (signature: `char* format_foo(int gpio, double rpm)`)
3. Add case to switch statements in `gpio.c:gpio_thread_fn()` and `measure.c:run_single_measurement()`
4. Add command-line option in `args.c`

### Modifying Measurement Behavior

Key locations:
- `gpio.c:gpio_measure_rpm()` - measurement algorithm and timing
- Warmup duration: line 181 (currently hard-coded to 1 second)
- Event counting loop: lines 218-238

### Adjusting Chip Auto-Detection

Modify `chip.c:chip_auto_detect()`:
- Currently tries gpiochip0-9
- Validates by checking line count (v2) or attempting line access (v1)

## Version Information

Version is set in main Makefile:
- `PKG_VERSION`: main version number
- `PKG_RELEASE`: release number
- Combined as `PKG_TAG` and embedded via `-DPKG_TAG` compile flag
