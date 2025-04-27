# Testing Recommendations

While the code is now well-prepared for both target environments, I recommend testing on actual hardware with:

- OpenWRT 23.05 with Kernel 5.15 (libgpiod v1)
- OpenWRT 24.10 with Kernel 6.6 (libgpiod v2)

Pay special attention to:

- Successful chip auto-detection
- Edge event detection for RPM measurements
- Memory usage during long-running operations (in watch mode)
