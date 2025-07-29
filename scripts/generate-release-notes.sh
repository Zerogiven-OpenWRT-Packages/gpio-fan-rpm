#!/bin/bash

# Generate dynamic release notes for GitHub releases
# Usage: ./scripts/generate-release-notes.sh [version]

set -e

CURRENT_VERSION=${1:-${GITHUB_REF#refs/tags/v}}

if [ -z "$CURRENT_VERSION" ]; then
    echo "Error: No version provided and GITHUB_REF not set"
    exit 1
fi

# Find the previous version tag
PREVIOUS_TAG=$(git describe --tags --abbrev=0 HEAD~1 2>/dev/null || echo "")

if [ -z "$PREVIOUS_TAG" ]; then
    # No previous tag found, get all commits since the beginning
    echo "No previous version found, showing all commits:"
    COMMIT_LOG=$(git log --oneline --no-merges --pretty=format:"- %s" | head -50)
    CHANGES_SECTION="### Changes in this release"
else
    # Get commits between previous tag and current tag
    PREVIOUS_VERSION=${PREVIOUS_TAG#v}
    echo "Generating release notes from $PREVIOUS_TAG to v$CURRENT_VERSION"
    
    COMMIT_LOG=$(git log --oneline --no-merges --pretty=format:"- %s" $PREVIOUS_TAG..HEAD)
    CHANGES_SECTION="### Changes since v$PREVIOUS_VERSION"
fi

if [ -z "$COMMIT_LOG" ]; then
    COMMIT_LOG="- No changes detected (tag-only release)"
fi

# Create dynamic release body
cat << EOF
## gpio-fan-rpm $CURRENT_VERSION

High-precision command-line utility for measuring fan RPM using GPIO edge detection on OpenWRT devices.

### Compatibility
- **OpenWRT 23.05**: libgpiod v1 support
- **OpenWRT 24.10**: libgpiod v2 support

### Installation
Download the appropriate package for your OpenWRT version and install with:
\`\`\`bash
opkg install <package-name>.ipk
\`\`\`

### Usage
\`\`\`bash
# Basic usage
gpio-fan-rpm --gpio=17

# Continuous monitoring
gpio-fan-rpm --gpio=17 --watch

# JSON output
gpio-fan-rpm --gpio=17 --json
\`\`\`

### Features
- High-precision RPM measurement using GPIO edge detection
- Support for both libgpiod v1 and v2 APIs
- Multiple output formats (human-readable, JSON, numeric, collectd)
- Continuous monitoring mode
- Multithreaded support for multiple GPIO pins
- Auto-detection of GPIO chips

$CHANGES_SECTION
$COMMIT_LOG

### Installation
\`\`\`bash
# For OpenWRT 23.05
opkg install gpio-fan-rpm-23.05-$CURRENT_VERSION.ipk

# For OpenWRT 24.10
opkg install gpio-fan-rpm-24.10-$CURRENT_VERSION.ipk
\`\`\`

See [CHANGELOG.md](https://github.com/$GITHUB_REPOSITORY/blob/main/CHANGELOG.md) for detailed changes.
EOF