# gpio-fan-rpm

`gpio-fan-rpm` is a lightweight C utility and OpenWRT package that measures fan RPM by counting falling edges on GPIO pins using sysfs and `poll()`. It supports multiple GPIO chips, flexible input formats, and various output modes including JSON and collectd.

Of course it could be compiled for other systems too, but this is especially for my Banana Pi R4 - OpenWRT setup. I am using this to monitor the fan's i've built in but you can use it for many things.

---

## 🔧 Features

- Supports multiple GPIOs from different GPIO chips
- Flexible GPIO syntax: `--gpio=17:gpiochip1` or anonymous `17:gpiochip1`
- Measurement via `poll()` on falling edges (no busy-wait)
- Output formats:
  - Human-readable (`GPIO 17: 2200 RPM`)
  - `--numeric` (plain RPM values, one per line)
  - `--json` (machine-readable)
  - `--collectd` (compatible with collectd exec plugin)
- Continuous monitoring via `--watch`
- Debug mode (`--debug`) with internal state logging
- Modular and portable C code
- OpenWRT package-ready with Makefile

---

## 🚀 Usage

```bash
gpio-fan-rpm [<gpio>[:chip]]... [options]
```

### Examples

```bash
gpio-fan-rpm 17 18
gpio-fan-rpm 10:gpiochip0 12:gpiochip1
gpio-fan-rpm --gpio=22 --duration=3 --json
gpio-fan-rpm 11 13 --collectd
```

---

## ⚙️ Options

| Option                   | Description |
|--------------------------|-------------|
| `-g`, `--gpio <n>`       | Relative GPIO number (can be repeated) |
| `--gpio=<n>:<chip>`      | GPIO number with specific GPIO chip |
| `-b`, `--gpio-base <n>`  | Set base GPIO (fallback if chip not used) |
| `-c`, `--gpiochip <name>`| Set default GPIO chip name |
| `-d`, `--duration <s>`   | Duration in seconds to measure (default: 2) |
| `-p`, `--pulses-per-rev` | Pulses per fan revolution (default: 2) |
| `-n`, `--numeric`        | Output plain RPM numbers (one per line) |
| `-j`, `--json`           | Output JSON objects |
| `--collectd`             | Output in collectd exec format |
| `-w`, `--watch`          | Continuously measure every &lt;duration&gt; seconds |
| `--debug`                | Enable debug logs |
| `-h`, `--help`           | Show help message |

---

## 📦 OpenWRT Integration

### Installation via local buildroot

1. Copy this package directory to your OpenWRT build tree:

   ```bash
   cp -r gpio-fan-rpm/ openwrt/package/utils/
   ```

2. Add it to your build config:

   ```bash
   make menuconfig
   # Select Utilities -> gpio-fan-rpm
   ```

3. Build and install:

   ```bash
   make package/gpio-fan-rpm/compile V=s
   ```

### Manual compilation (for testing)

```bash
make
scp gpio-fan-rpm root@<router-ip>:/usr/sbin/
```

---

## 📡 collectd Integration

Example for `collectd.conf`:

```conf
<Plugin exec>
  Exec "nobody" "/usr/sbin/gpio-fan-rpm" "17" "--collectd"
</Plugin>
```

---

## 📄 License

MIT License

---

## 👤 Maintainer

Open Source Community – feel free to contribute!
