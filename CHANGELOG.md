# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

All notable changes to this project will be documented in this file.

## [Unreleased]
## [1.0.0] - 2025-07-29

### Added
- Version 1.0.0 release

### Changed
- Updated version to 1.0.0

### Fixed
- None

### Removed
- None



### Added
- **Initial release**: High-precision fan RPM measurement using GPIO edge detection
- **Cross-version support**: OpenWRT 23.05 (libgpiod v1) and 24.10 (libgpiod v2)
- **Multiple output formats**: Human-readable, JSON, numeric, and collectd
- **Continuous monitoring**: Watch mode for real-time RPM tracking
- **Multithreaded support**: Multiple GPIO pins simultaneously
- **Auto-detection**: Automatic GPIO chip detection
- **Configurable parameters**: Duration and pulses per revolution
- **UCI configuration support**: Configurable default values via `/etc/config/gpio-fan-rpm`
- **Performance notes**: Accuracy, speed limits, and measurement recommendations
- **Integration examples**: collectd and custom OpenWRT build instructions
- **Progress indicators**: Debug output for long measurements
- **Common fan pulse counts table**: Reference for different fan types
- **Comprehensive installation guide**: Step-by-step setup instructions
- **Detailed troubleshooting guide**: Common issues and solutions
- **Retry logic**: Automatic retry on temporary GPIO failures
- **Resource limits**: Protection against excessive resource usage
- **Keyboard controls**: Press 'q' to quit watch mode gracefully

### Changed
- **Default pulses per revolution**: Changed from 2 to 4 (Noctua-friendly)
- **Enhanced version output**: Now shows libgpiod version (detected at compile time)
- **Improved documentation**: Added comprehensive installation and troubleshooting guides
- **Enhanced signal handling**: Added SIGTERM support for graceful shutdown
- **Better error recovery**: Improved GPIO error handling with detailed debug messages
- **Enhanced input validation**: More robust argument parsing with better error messages
- **Improved resource cleanup**: Better memory management and GPIO resource cleanup
- **CLI improvements**: Simplified help output, better error messages with actionable suggestions
- **Environment variable support**: Added support for GPIO_FAN_RPM_DURATION, GPIO_FAN_RPM_PULSES, and DEBUG env vars
- **Measurement timing**: Simplified to 1-second warmup + measurement duration for better responsiveness
- **Minimum duration**: Enforced 2-second minimum for accurate measurements

### Fixed
- **Debug output**: Added more detailed measurement information
- **Documentation**: Updated examples and troubleshooting for new default pulses
- **Error handling**: Better recovery from GPIO errors and timeouts
- **Memory leaks**: Improved resource cleanup and memory management
- **Input validation**: More robust argument parsing and validation
- **Signal handling**: Fixed 0 RPM output on program exit/interruption
- **Output formatting**: RPM values now output as integers instead of decimals
- **Double output**: Fixed duplicate output in single measurement mode
- **Terminal output**: Added newline after interrupt for cleaner output

### Planned
- Unit tests
- Additional hardware compatibility 