# GPIO Fan RPM for OpenWRT

High-precision command-line utility for measuring fan RPM using GPIO edge detection on OpenWRT.

## Features

- **High Precision**: Event-driven timerfd timing with ±1 RPM accuracy for stable fans
- **Multiple Formats**: Human-readable, JSON, numeric, and collectd output
- **Watch Mode**: Continuous monitoring with graceful quit (press 'q')
- **Multi-GPIO**: Parallel measurement of multiple fans simultaneously
- **Efficient**: Event-driven design, minimal CPU and memory usage

## Installation

**Pre-built package:**
```bash
opkg install gpio-fan-rpm-*.ipk
```

**Custom build:**
```bash
git clone https://github.com/Zerogiven-OpenWRT-Packages/gpio-fan-rpm.git package/utils/gpio-fan-rpm
make menuconfig  # Navigate to: Utilities → gpio-fan-rpm
make package/gpio-fan-rpm/compile V=s
```

**Requirements:** OpenWRT 24.10, libgpiod, libjson-c (auto-installed), fan with tachometer output

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

**Measurement behavior:**
- Total duration = 1s warmup + (duration-1)s measurement
- Watch mode: press 'q' to quit, Ctrl+C to interrupt
- Default: 4 pulses/revolution (common for PC fans)

**Common fan pulse counts:** Noctua NF-A6x25 (4), most Noctua (2), PC fans (2-4)

**UCI configuration:** Defaults configurable via `/etc/config/gpio-fan-rpm`

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

| Issue | Solution |
|-------|----------|
| Permission denied | Run as root: `sudo gpio-fan-rpm --gpio=17` |
| Wrong RPM (e.g., 6000 instead of 3000) | Adjust `--pulses`: try `--pulses=2` instead of `--pulses=4` |
| GPIO chip not found | Check chips: `ls /dev/gpiochip*` or `gpioinfo` |
| No events detected | Verify fan spinning, check tachometer wire, try different GPIO |

**Debug mode:** Use `--debug` for detailed timing and event information

## Technical Details

- **Timing**: Event-driven timerfd for precise measurements (eliminates polling overhead)
- **Architecture**: Multithreaded (one thread per GPIO), edge detection (both rising/falling)
- **Signals**: Graceful shutdown on SIGINT/SIGTERM

**collectd integration example:**
```conf
LoadPlugin exec
<Plugin exec>
    Exec "nobody" "/usr/sbin/gpio-fan-rpm" "--gpio=17" "--numeric"
</Plugin>
```

## License

LGPL-3.0-or-later. See [LICENSE](LICENSE) and [NOTICE](NOTICE) for third-party attributions (libgpiod, libjson-c).

---

**Developed for OpenWRT** • [Report Issues](https://github.com/Zerogiven-OpenWRT-Packages/gpio-fan-rpm/issues)
