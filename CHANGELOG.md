# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-01-27

### Added
- Initial release of gpio-fan-rpm utility
- Support for OpenWRT 23.05 (libgpiod v1.6.x) and 24.10 (libgpiod v2.1.x)
- High-precision RPM measurement using GPIO edge detection
- Multiple output formats: human-readable, JSON, numeric, and collectd
- Continuous monitoring mode with `--watch` option
- Multithreaded support for multiple GPIO pins
- Auto-detection of GPIO chips
- Configurable measurement duration and pulses per revolution
- Comprehensive error handling and validation
- Debug mode for troubleshooting
- Parallel measurement with ordered output for multiple GPIOs

### Features
- **Cross-version compatibility**: Automatic detection and adaptation to libgpiod v1/v2
- **Modular architecture**: Clean separation of chip, line, and GPIO operations
- **Robust error handling**: Graceful degradation and informative error messages
- **Memory efficient**: Minimal footprint suitable for embedded systems
- **Signal handling**: Graceful shutdown on SIGINT (Ctrl+C)
- **Thread safety**: Proper mutex protection for output formatting
- **Parallel measurement**: Multiple GPIOs measured simultaneously with ordered output

### Technical Details
- **Language**: C99 with GNU extensions
- **Dependencies**: libgpiod, libjson-c, librt, libpthread
- **Build system**: OpenWRT package with Makefile and Kconfig
- **License**: GPL-3.0-only
- **Architecture**: Multithreaded with modular design

### Documentation
- Comprehensive README with usage examples
- API documentation with Doxygen comments
- Troubleshooting guide
- Technical architecture documentation
- License and third-party attribution

## [Unreleased]

### Planned
- Support for additional output formats
- Configuration file support
- Logging system
- Performance optimizations
- Additional hardware compatibility 