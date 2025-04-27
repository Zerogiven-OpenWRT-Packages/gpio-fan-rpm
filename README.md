# gpio-fan-rpm

> Modular and well-commented fan RPM measurement utility for OpenWRT (libgpiod v1/v2 compatible)

This command-line tool allows you to measure the speed (in RPM) of one or more fans that emit pulse signals on a GPIO pin (e.g. Noctua fans with a tachometer wire).  
It supports both **libgpiod v1** and **libgpiod v2** APIs and offers edge event detection via multithreading for high precision.

---

## Features

- Edge-event-based RPM detection using **libgpiod** (v1 or v2)
- Multithreaded measurement for multiple GPIOs in parallel
- Warm-up phase to avoid initial inaccuracies
- Watch mode for continuous monitoring
- JSON, numeric and collectd-compatible output
- Auto-detection of GPIO chip if not specified
- Fully written in portable C (no C++ dependencies)
- Compatible with OpenWRT 23.05+ and 24.10+

---

## Usage

```bash
gpio-fan-rpm [options]
```

### Options

| Option             | Description                               |
| ------------------ | ----------------------------------------- |
| `--gpio=N`, `-g N` | GPIO number to measure (relative to chip) |
| `--chip=NAME`      | GPIO chip name (default: `gpiochip0`)     |
| `--duration=SEC`   | Total measurement duration (default: `2`) |
| `--pulses=N`       | Pulses per revolution (default: `2`)      |
| `--numeric`        | Output RPM values only (one per line)     |
| `--json`           | Output RPM values as JSON                 |
| `--collectd`       | Output in collectd PUTVAL format          |
| `--debug`          | Enable debug output                       |
| `--watch`, `-w`    | Continuously repeat measurements          |
| `--help`, `-h`     | Show help message                         |

---

## Timing Behavior

⚠️ **Important timing information**:

- The `--duration` value represents the **total time** used per measurement cycle.
- Each measurement run is split into two phases:
  - **Warm-up phase**: `duration / 2` — data collected but discarded (stabilization)
  - **Actual measurement**: `duration / 2` — data used for output
- In **watch mode**, the warm-up phase is **only used on the first cycle**.
  All following loops use only `duration / 2` as the actual measurement time.

---

## Example

```bash
gpio-fan-rpm --gpio=17 --pulses=4 --duration=4
```

- This performs:
  - 2 seconds warm-up
  - 2 seconds actual measurement
  - then prints the RPM for GPIO 17

---

## Dependencies

- [libgpiod v1 or v2](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/) — must be installed and linked at compile time.
- Build tested on OpenWRT 23+ with `libgpiod 2.1.3`.

---

## Build

```bash
make
```

This will generate the `gpio-fan-rpm` binary.

---

## License

GPL License.  
Copyright © 2025 CSoellinger.  
Developed by the Open Source Community.
