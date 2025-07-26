# GitHub Workflows

This directory contains GitHub Actions workflows for the gpio-fan-rpm project.

## Available Workflows

### Build Test (`build-test.yml`)

A comprehensive build testing workflow that compiles the gpio-fan-rpm package for different OpenWRT versions and architectures using the OpenWRT SDK.

### Quick Build Test (`quick-build-test.yml`)

A simplified build testing workflow for quick validation during development. Focuses on the most common configuration (x86_64) for faster testing with optional verbose output.

#### Features

**Build Test Workflow:**
- **Manual Trigger Only**: Uses `workflow_dispatch` to prevent automatic runs
- **Multiple OpenWRT Versions**: Tests both OpenWRT 23.05 and 24.10
- **Multiple Architectures**: Supports x86_64, aarch64, and ARM Cortex-A7
- **SDK-based Builds**: Uses official OpenWRT SDK for accurate testing
- **Artifact Upload**: Saves built packages as workflow artifacts
- **Detailed Logging**: Provides comprehensive build information
- **Package Analysis**: Additional checks on built packages
- **Comprehensive Testing**: Full validation for releases

**Quick Build Test Workflow:**
- **Manual Trigger Only**: Uses `workflow_dispatch` to prevent automatic runs
- **Single Architecture**: Focuses on x86_64 (most common)
- **Single Version**: Tests one OpenWRT version at a time
- **Optional Verbose Output**: Can toggle detailed build logs
- **Fast Execution**: Optimized for quick development feedback
- **Basic Artifact Upload**: Saves built packages
- **Development Focused**: Ideal for iterative development

#### Usage

**Build Test Workflow:**
1. **Navigate to Actions**: Go to the "Actions" tab in your GitHub repository
2. **Select Workflow**: Choose "Build Test" from the workflow list
3. **Manual Trigger**: Click "Run workflow" button
4. **Configure Options**:
   - **OpenWRT Version**: Choose `both`, `23.05`, or `24.10`
   - **Architecture**: Choose `x86_64`, `aarch64`, `arm_cortex-a7`, or `mipsel_24kc`
5. **Run**: Click "Run workflow" to start the build process

**Quick Build Test Workflow:**
1. **Navigate to Actions**: Go to the "Actions" tab in your GitHub repository
2. **Select Workflow**: Choose "Quick Build Test" from the workflow list
3. **Manual Trigger**: Click "Run workflow" button
4. **Configure Options**:
   - **OpenWRT Version**: Choose `23.05` or `24.10`
   - **Verbose Output**: Toggle detailed build logs (optional)
5. **Run**: Click "Run workflow" to start the build process (x86_64 only)

#### Workflow Comparison

| Feature | Build Test | Quick Build Test |
|---------|------------|------------------|
| **Purpose** | Release validation | Development testing |
| **Architectures** | Multiple (x86_64, aarch64, ARM) | Single (x86_64 only) |
| **OpenWRT Versions** | Both 23.05 & 24.10 | Single version |
| **Build Output** | Always verbose | Optional verbose |
| **Package Analysis** | Comprehensive | Basic |
| **Execution Time** | Longer (matrix builds) | Faster (single build) |
| **Use Case** | Pre-release testing | Iterative development |

#### Supported Combinations (Build Test Only)

| OpenWRT Version | Architecture | Target | Subtarget | SDK Name |
|----------------|--------------|--------|-----------|----------|
| 23.05 | x86_64 | x86 | 64 | x86-64 |
| 23.05 | aarch64 | bcm27xx | bcm2711 | bcm27xx-bcm2711 |
| 23.05 | arm_cortex-a7 | sunxi | cortexa7 | sunxi-cortexa7 |
| 24.10 | x86_64 | x86 | 64 | x86-64 |
| 24.10 | aarch64 | bcm27xx | bcm2711 | bcm27xx-bcm2711 |
| 24.10 | arm_cortex-a7 | sunxi | cortexa7 | sunxi-cortexa7 |

#### Workflow Steps

1. **Matrix Compatibility Check**: Validates if the matrix combination should be built
2. **Repository Checkout**: Clones the repository with full history
3. **Build Environment Setup**: Installs required packages on Ubuntu
4. **SDK Download**: Downloads the appropriate OpenWRT SDK
5. **SDK Environment Setup**: Configures the SDK for building
6. **Package Source Copy**: Copies the package source to the SDK
7. **Package Build**: Compiles the gpio-fan-rpm package
8. **Artifact Upload**: Saves built packages as workflow artifacts
9. **Build Summary**: Provides detailed build results

#### Output

- **Success**: Generated `.ipk` packages and build logs
- **Artifacts**: Built packages available for download
- **Summary**: Detailed build status in workflow summary

#### Dependencies

The workflow automatically installs the following packages on Ubuntu:
- build-essential, clang, flex, bison
- g++, gawk, gcc-multilib, g++-multilib
- gettext, git, libncurses5-dev, libssl-dev
- python3-setuptools, rsync, swig, unzip, zlib1g-dev
- file, wget, time

#### SDK Configuration

The workflow configures the OpenWRT SDK with:
- Minimal build configuration
- Required dependencies (libgpiod, libjson-c, libpthread)
- Target-specific optimizations
- Musl libc support

#### Troubleshooting

**Build Failures**:
- Check the build logs for specific error messages
- Verify that all dependencies are properly declared in the Makefile
- Ensure the package structure follows OpenWRT conventions

**SDK Download Issues**:
- Verify the SDK URL is correct for the target architecture
- Check if the OpenWRT version is still supported
- Ensure the target/subtarget combination is valid

**Matrix Filtering**:
- The workflow uses conditional logic to filter matrix combinations
- Only builds the requested OpenWRT version and architecture
- Skips incompatible combinations gracefully

#### Future Enhancements

- **Automated Triggers**: Add triggers for version releases
- **Additional Architectures**: Support for more target platforms
- **Integration Tests**: Add runtime testing of built packages
- **Performance Metrics**: Track build times and package sizes
- **Dependency Analysis**: Verify package dependencies

## Workflow Development

### Adding New Workflows

1. Create a new `.yml` file in `.github/workflows/`
2. Follow the existing workflow structure
3. Add appropriate documentation
4. Test with manual triggers first

### Modifying Existing Workflows

1. Update the workflow file
2. Test changes with manual triggers
3. Update this README if needed
4. Consider backward compatibility

### Best Practices

- **Manual Triggers**: Use `workflow_dispatch` for testing workflows
- **Matrix Builds**: Leverage GitHub's matrix strategy for multiple combinations
- **Artifact Management**: Upload and retain build artifacts appropriately
- **Error Handling**: Provide clear error messages and failure information
- **Documentation**: Keep this README updated with workflow changes 