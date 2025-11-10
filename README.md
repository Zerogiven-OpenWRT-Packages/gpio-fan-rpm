# GPIO Fan RPM for OpenWRT

A high-precision command-line utility for measuring fan RPM using GPIO edge detection on OpenWRT devices. Compatible with both OpenWRT 23.05 (libgpiod v1) and 24.10 (libgpiod v2).

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
  - [Quick Installation](#quick-installation)
  - [Custom OpenWRT Build](#custom-openwrt-build)
- [Usage](#usage)
  - [Basic Usage](#basic-usage)
  - [Command-Line Options](#command-line-options)
  - [Examples](#examples)
  - [Performance Notes](#performance-notes)
  - [Measurement Behavior](#measurement-behavior)
- [Configuration](#configuration)
  - [UCI Configuration](#uci-configuration)
  - [Common Fan Pulse Counts](#common-fan-pulse-counts)
- [Output Formats](#output-formats)
- [Troubleshooting](#troubleshooting)
  - [Common Issues](#common-issues)
  - [Debug Mode](#debug-mode)
- [Technical Details](#technical-details)
  - [libgpiod Version Compatibility](#libgpiod-version-compatibility)
  - [Architecture](#architecture)
  - [Integration Examples](#integration-examples)
- [License](#license)

## Features

- **High Precision**: Uses edge detection for accurate RPM measurement (±1 RPM for stable fans)
- **Cross-version Support**: Works with both libgpiod v1 (OpenWRT 23.05) and v2 (OpenWRT 24.10)
- **Multiple Output Formats**: Human-readable, JSON, numeric, and collectd-compatible output
- **Continuous Monitoring**: Watch mode for real-time RPM tracking
- **Multithreaded**: Supports multiple GPIO pins simultaneously
- **UCI Configuration**: Configurable defaults via OpenWRT's UCI system
- **Resource Efficient**: Minimal CPU and memory usage

## Requirements

- OpenWRT 23.05 (Kernel 5.15) or 24.10 (Kernel 6.6)
- libgpiod library (automatically installed as dependency)
- libjson-c library (for JSON output format)
- A fan with tachometer output connected to a GPIO pin

## Installation

### Quick Installation

Download the appropriate package for your OpenWRT version:

```bash
# For OpenWRT 23.05
opkg install gpio-fan-rpm-23.05-*.ipk

# For OpenWRT 24.10
opkg install gpio-fan-rpm-24.10-*.ipk
```

### Custom OpenWRT Build

1. Clone this repository into `package/utils/`:
   ```bash
   git clone https://github.com/Zerogiven-OpenWRT-Packages/gpio-fan-rpm.git package/utils/gpio-fan-rpm
   ```

2. Update and install feeds:
   ```bash
   ./scripts/feeds update -a
   ./scripts/feeds install gpio-fan-rpm
   ```

3. Enable in menuconfig:
   ```bash
   make menuconfig
   # Navigate to: Utilities → gpio-fan-rpm → [M] or [*]
   ```

4. Build the package:
   ```bash
   make package/gpio-fan-rpm/compile V=s
   ```

## Usage

### Basic Usage

```bash
gpio-fan-rpm --gpio=17
```

### Command-Line Options

| Option | Description |
|--------|-------------|
| `-g, --gpio=N` | GPIO number to measure (required, can be repeated) |
| `-c, --chip=NAME` | GPIO chip name (default: auto-detect) |
| `-d, --duration=SEC` | Measurement duration in seconds (default: 2, min: 2) |
| `-p, --pulses=N` | Pulses per revolution (default: 4) |
| `-w, --watch` | Continuously monitor RPM |
| `-n, --numeric` | Output only the RPM value |
| `-j, --json` | Output in JSON format |
| `--collectd` | Output in collectd format |
| `--debug` | Enable debug output |
| `-h, --help` | Show help message |
| `-v, --version` | Show version information |

### Examples

```bash
# Basic usage with default settings (4 pulses per revolution)
gpio-fan-rpm --gpio=17

# Custom chip and pulses per revolution
gpio-fan-rpm --gpio=17 --chip=gpiochip1 --pulses=4

# Continuous monitoring with 4-second measurement interval
gpio-fan-rpm --gpio=17 --duration=4 --watch

# In watch mode, press 'q' to quit gracefully or Ctrl+C to interrupt

# Multiple output formats
gpio-fan-rpm --gpio=17 --json                    # JSON output
gpio-fan-rpm --gpio=17 --numeric                 # Numeric only
gpio-fan-rpm --gpio=17 --collectd                # collectd format
RPM=$(gpio-fan-rpm --gpio=17 --numeric)          # Capture in variable

# Multiple GPIO pins simultaneously
gpio-fan-rpm --gpio=17 --gpio=18 --gpio=19

# Debug output to see measurement details
gpio-fan-rpm --gpio=17 --debug
```

### Performance Notes

- **Accuracy**: ±1 RPM for stable fans
- **Speed range**: ~100 RPM (minimum) to ~10,000 RPM (maximum)
- **Measurement time**: 1-10 seconds recommended (longer = more accurate)
- **CPU usage**: Minimal (interrupt-driven edge detection)

### Measurement Behavior

The `--duration` value represents the total time per measurement cycle:
- **Warmup phase**: 1 second (stabilization, no output)
- **Measurement phase**: `duration-1` seconds (actual measurement)

In watch mode, warmup occurs only once before the loop starts.

**Watch Mode Controls:**
- Press `q` to quit gracefully
- Press `Ctrl+C` to interrupt immediately

## Configuration

### UCI Configuration

Configure default values via `/etc/config/gpio-fan-rpm`:

```bash
# Set default duration to 4 seconds
uci set gpio-fan-rpm.defaults.duration=4

# Set default pulses to 2 for older Noctua fans
uci set gpio-fan-rpm.defaults.pulses=2

# Commit changes
uci commit gpio-fan-rpm
```

### Common Fan Pulse Counts

| Fan Type | Pulses per Revolution | Default |
|----------|---------------------|---------|
| Noctua NF-A6x25 | 4 | ✅ |
| Noctua (most models) | 2 | |
| PC case fans | 2-4 | |
| Server fans | 2-8 | |

## Output Formats

```bash
# Human-readable (default)
GPIO17: RPM: 1235

# JSON (single GPIO)
{"gpio": 17, "rpm": 1235}

# JSON array (multiple GPIOs)
[{"gpio": 17, "rpm": 1235}, {"gpio": 18, "rpm": 2044}]

# Numeric only
1235

# collectd format
PUTVAL "hostname/gpio-fan-17/gauge-rpm" interval=2 1719272100:1235
```

## Troubleshooting

### Common Issues

**Permission Denied**
```bash
Error: cannot open chip for GPIO 17
```
**Solution**: Run as root or add user to gpio group
```bash
sudo gpio-fan-rpm --gpio=17
# or
usermod -a -G gpio $USER
```

**No Events Detected**
```bash
Warning: No events during warmup
```
**Solutions**:
- Verify fan is spinning
- Check tachometer wire connection
- Ensure GPIO pin is not used by other processes
- Try different GPIO pin

**Incorrect RPM Readings**
```bash
# Expected: 3000 RPM, Got: 6000 RPM
```
**Solution**: Adjust `--pulses` parameter. If you get ~6000 RPM with `--pulses=4` but expect ~3000 RPM, try `--pulses=2`

**GPIO Chip Not Found**
```bash
Error: cannot open chip
```
**Solution**: Check available GPIO chips:
```bash
ls -l /dev/gpiochip*
gpioinfo
```

**High CPU Usage**
**Solution**: Use longer measurement duration:
```bash
gpio-fan-rpm --gpio=17 --duration=5
```

### Debug Mode

Use `--debug` for detailed information:
```bash
gpio-fan-rpm --gpio=17 --debug
```

Shows GPIO chip detection, event counting details, timing information, and error messages.

## Technical Details

### libgpiod Version Compatibility

Automatically detects and adapts to available libgpiod version:
- **OpenWRT 23.05**: Uses libgpiod v1.6.x API
- **OpenWRT 24.10**: Uses libgpiod v2.1.x API

### Architecture

- **Multithreaded design**: Each GPIO pin runs in its own thread
- **Edge detection**: Uses both rising and falling edge detection
- **Memory efficient**: Minimal footprint for embedded systems
- **Signal handling**: Graceful shutdown on SIGINT/SIGTERM
- **Auto-detection**: Intelligent chip detection across hardware

### Integration Examples

**collectd Integration**
```conf
# Add to /etc/collectd.conf
LoadPlugin exec
<Plugin exec>
    Exec "nobody" "/usr/sbin/gpio-fan-rpm" "--gpio=17" "--numeric"
</Plugin>
```

## License

This project is licensed under the GNU General Public License v3.0 only (GPL-3.0-only).
See the [LICENSE](LICENSE) file for details.

### Third-Party Licenses

- **libgpiod**: LGPL-2.1-or-later (Copyright: 2017-2023 Bartosz Golaszewski)
- **libjson-c**: MIT License (Copyright: 2009-2012 Eric Haszlakiewicz)
- **C Library**: LGPL-2.1-or-later (glibc) / MIT License (musl)

See [NOTICE](NOTICE) file for complete attribution details.

---

Developed for OpenWRT by CSoellinger