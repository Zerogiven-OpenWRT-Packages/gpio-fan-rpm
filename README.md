# GPIO Fan RPM for OpenWRT

A high-precision command-line utility for measuring fan RPM using GPIO edge detection on OpenWRT devices. Compatible with both OpenWRT 23.05 (libgpiod v1) and 24.10 (libgpiod v2).

## 🚀 Features

- **High Precision**: Uses edge detection for accurate RPM measurement
- **Cross-version Support**: Works with both libgpiod v1 and v2
- **Efficient**: Lightweight and optimized for embedded systems
- **Multiple Output Formats**: Human-readable, JSON, numeric, and collectd-compatible output
- **Configurable**: Adjustable measurement duration and pulses per revolution
- **Continuous Monitoring**: Watch mode for ongoing RPM monitoring
- **Multithreaded**: Supports multiple GPIO pins simultaneously
- **Robust Error Handling**: Graceful degradation and informative error messages

## 📋 Requirements

- OpenWRT 23.05 (Kernel 5.15) or 24.10 (Kernel 6.6)
- libgpiod library (automatically installed as a dependency)
- libjson-c library (for JSON output format)
- A fan with tachometer output connected to a GPIO pin

## 🛠️ Installation

### Using opkg (Recommended)

```bash
opkg update
opkg install gpio-fan-rpm
```

### Manual Build

1. Clone this repository to your OpenWRT build system
2. Add the package to your build configuration:
   ```bash
   make menuconfig
   # Navigate to: Utilities -> gpio-fan-rpm
   # Select with 'M' for module or 'Y' to include in firmware
   ```
3. Build the package:
   ```bash
   make package/gpio-fan-rpm/compile V=s
   ```
4. Install the generated package from `bin/packages/<arch>/base/`

## 📖 Usage

### Basic Usage

```bash
gpio-fan-rpm --gpio=17
```

### Command-Line Options

| Option | Description |
|--------|-------------|
| `-g, --gpio=N` | GPIO number to measure (required, can be repeated) |
| `-c, --chip=NAME` | GPIO chip name (default: auto-detect) |
| `-d, --duration=SEC` | Measurement duration in seconds (default: 2) |
| `-p, --pulses=N` | Pulses per revolution (default: 2) |
| `-w, --watch` | Continuously monitor RPM |
| `-n, --numeric` | Output only the RPM value |
| `-j, --json` | Output in JSON format |
| `--collectd` | Output in collectd format |
| `--debug` | Enable debug output |
| `-h, --help` | Show help message |
| `-v, --version` | Show version information |

### Examples

```bash
# Basic usage with default settings
gpio-fan-rpm --gpio=17

# Specify custom chip and pulses per revolution
gpio-fan-rpm --gpio=17 --chip=gpiochip1 --pulses=4

# Continuous monitoring with 4-second measurement interval
gpio-fan-rpm --gpio=17 --duration=4 --watch

# JSON output for scripts
gpio-fan-rpm --gpio=17 --json

# Simple numeric output (useful for scripts)
RPM=$(gpio-fan-rpm --gpio=17 --numeric)

# Multiple GPIO pins simultaneously
gpio-fan-rpm --gpio=17 --gpio=18 --gpio=19

# Multiple GPIO pins with JSON array output
gpio-fan-rpm --gpio=17 --gpio=18 --json

# Debug output to see measurement details
gpio-fan-rpm --gpio=17 --debug

# collectd monitoring format
gpio-fan-rpm --gpio=17 --collectd
```

## 📊 Output Formats

### Human-Readable (Default)
```
GPIO17: RPM: 1235
```

### JSON (Single GPIO)
```json
{"gpio": 17, "rpm": 1235}
```

### JSON Array (Multiple GPIOs)
```json
[{"gpio": 17, "rpm": 1235}, {"gpio": 18, "rpm": 2044}]
```

### Numeric
```
1235
```

### collectd
```
PUTVAL "hostname/gpio-fan-17/gauge-rpm" interval=2 1719272100:1235
```





## ⏱️ Measurement Behavior

### Timing Details

The `--duration` value represents the **total time** per measurement cycle:

- **Warmup phase**: `duration/2` seconds (stabilization, no output)
- **Measurement phase**: `duration/2` seconds (actual measurement)

In **watch mode**, the warmup occurs only once before the loop starts. All subsequent iterations use only `duration/2` for measurement.

### Accuracy Considerations

- **Minimum duration**: 1 second recommended for accurate readings
- **Pulse detection**: Both rising and falling edges are detected
- **Error handling**: Graceful degradation on timeouts or errors
- **Debug mode**: Use `--debug` to see detailed timing information

## Troubleshooting

1. **No RPM reading**
   - Verify the GPIO pin is correctly connected to the fan's tachometer wire
   - Check if the GPIO pin is available and not used by another process
   - Try running with `--debug` flag for more information
   - Ensure you have proper permissions (run as root if needed)

2. **Incorrect RPM values**
   - Verify the `--pulses` parameter matches your fan's specification (typically 2 for most PC fans)
   - Try increasing the measurement duration with `--duration`
   - Check if the fan is actually spinning

3. **Permission denied**
   - Run the command as root: `sudo gpio-fan-rpm --gpio=17`
   - Or add your user to the `gpio` group and ensure proper udev rules are in place

4. **Build issues**
   - Ensure libgpiod and libjson-c are available in your OpenWRT build environment
   - Check that you're building for the correct OpenWRT version (23.05 or 24.10)

## Technical Details

### libgpiod Version Compatibility

This tool automatically detects and adapts to the libgpiod version available:

- **OpenWRT 23.05**: Uses libgpiod v1.6.x API
- **OpenWRT 24.10**: Uses libgpiod v2.1.x API

The detection is done at build time by the OpenWRT build system, and the appropriate API calls are compiled in.

### API Compatibility

The tool uses a modular architecture that provides compatibility with both libgpiod versions:

- **Chip Operations**: Unified interface for opening/closing GPIO chips
- **Line Operations**: Version-specific line request and event handling
- **Event Detection**: Proper edge event detection for both APIs
- **Auto-detection**: Intelligent chip detection across different hardware

### Architecture

- **Multithreaded design**: Each GPIO pin runs in its own thread for parallel measurement
- **Edge detection**: Uses both rising and falling edge detection for maximum accuracy
- **Memory efficient**: Minimal memory footprint suitable for embedded systems
- **Signal handling**: Graceful shutdown on SIGINT (Ctrl+C)

## License

This project is licensed under the GNU General Public License v3.0 only (GPL-3.0-only).
See the [LICENSE](LICENSE) file for the full license text.

### Third-Party Licenses

This software uses the following third-party libraries:

- **libgpiod**
  - License: LGPL-2.1-or-later
  - Copyright: 2017-2023 Bartosz Golaszewski
  - Source: https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/

- **libjson-c**
  - License: MIT License
  - Copyright: 2009-2012 Eric Haszlakiewicz
  - Source: https://github.com/json-c/json-c

- **C Library** (providing librt and libpthread)
  - glibc: LGPL-2.1-or-later
  - musl: MIT License

See the [NOTICE](NOTICE) file for complete attribution details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.

When contributing to this project, you agree to license your contributions under
the same license as this project (GPL-3.0-only).

## Credits

Developed for OpenWRT by CSoellinger