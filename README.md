# GPIO Fan RPM

A high-precision command-line utility for measuring fan RPM using GPIO edge detection on OpenWrt.

[![OpenWrt](https://img.shields.io/badge/OpenWrt-24.10.x-green.svg)](https://openwrt.org/)

This is an OpenWrt package for [https://github.com/CSoellinger/gpio-fan-rpm](https://github.com/CSoellinger/gpio-fan-rpm).

If you need support for OpenWrt 23.05 install version 1.x.

## Features

- **High Precision**: Event-driven timerfd timing with ±1 RPM accuracy for stable fans
- **Multiple Formats**: Human-readable, JSON, numeric, and collectd output
- **Watch Mode**: Continuous monitoring with graceful quit (press 'q')
- **Multi-GPIO**: Parallel measurement of multiple fans simultaneously
- **Efficient**: Event-driven design, minimal CPU and memory usage

## Requirements

- OpenWrt 24.10.x or later
- libgpiod
- Fan with tachometer output

## Installation

### From Package Feed

You can setup this package feed to install and update it with opkg:

[https://github.com/Zerogiven-OpenWRT-Packages/package-feed](https://github.com/Zerogiven-OpenWRT-Packages/package-feed)

### From IPK Package

```bash
opkg install gpio-fan-rpm-*.ipk
```

### From Source

```bash
git clone https://github.com/Zerogiven-OpenWRT-Packages/gpio-fan-rpm.git package/utils/gpio-fan-rpm
make menuconfig  # Navigate to: Utilities → gpio-fan-rpm
make package/gpio-fan-rpm/compile V=s
```

## Usage

```bash
gpio-fan-rpm --gpio=17                        # Basic measurement
gpio-fan-rpm --gpio=17 --duration=4 --watch   # Continuous monitoring
gpio-fan-rpm --gpio=17 --json                 # JSON output
gpio-fan-rpm --gpio=17 --gpio=18              # Multiple fans
```

### Options

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

### Measurement Behavior

- Total duration = 1s warmup + (duration-1)s measurement
- Watch mode: press 'q' to quit, Ctrl+C to interrupt
- Default: 4 pulses/revolution (common for PC fans)
- Common fan pulse counts: Noctua NF-A6x25 (4), most Noctua (2), PC fans (2-4)

### Output Formats

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

| Issue | Solution |
|-------|----------|
| Permission denied | Run as root: `sudo gpio-fan-rpm --gpio=17` |
| Wrong RPM (e.g., 6000 instead of 3000) | Adjust `--pulses`: try `--pulses=2` instead of `--pulses=4` |
| GPIO chip not found | Check chips: `ls /dev/gpiochip*` or `gpioinfo` |
| No events detected | Verify fan spinning, check tachometer wire, try different GPIO |

Use `--debug` for detailed timing and event information.

### collectd Integration

```conf
LoadPlugin exec
<Plugin exec>
    Exec "nobody" "/usr/sbin/gpio-fan-rpm" "--gpio=17" "--numeric"
</Plugin>
```

## Technical Details

- **Timing**: Event-driven timerfd for precise measurements (eliminates polling overhead)
- **Architecture**: Multithreaded (one thread per GPIO), edge detection (both rising/falling)
- **Signals**: Graceful shutdown on SIGINT/SIGTERM
