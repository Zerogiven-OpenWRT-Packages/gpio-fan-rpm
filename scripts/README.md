# Scripts Directory

This directory contains utility scripts for the gpio-fan-rpm project.

## Available Scripts

### update-version.sh

A comprehensive version update script that automates the release process.

**Features:**
- Updates version in `Makefile`
- Adds changelog entries to `CHANGELOG.md`
- Creates git commit and annotated tag
- Pushes changes to remote repository
- Supports dry-run mode for testing
- Validates semantic versioning format
- Checks git repository status

### example-workflow.sh

A demonstration script showing how to use the version update script in a typical workflow.

**Features:**
- Interactive workflow with confirmation prompts
- Demonstrates different version types (patch, minor, major)
- Shows step-by-step process
- Includes safety checks and dry-run preview

**Usage:**
```bash
# Patch release workflow
./scripts/example-workflow.sh patch

# Minor release workflow
./scripts/example-workflow.sh minor

# Major release workflow
./scripts/example-workflow.sh major

# Custom version workflow
./scripts/example-workflow.sh custom 1.0.0-alpha.1
```

**Usage:**
```bash
# Basic usage
./scripts/update-version.sh 1.0.1

# With custom commit message
./scripts/update-version.sh 2.0.0 --message "Major release with new features"

# Dry run to see what would be changed
./scripts/update-version.sh 1.1.0 --dry-run

# Force update even with uncommitted changes
./scripts/update-version.sh 1.0.2 --force

# Only create git tag (don't update files)
./scripts/update-version.sh 1.0.3 --tag-only
```

**Options:**
- `-h, --help`: Show help message
- `-d, --dry-run`: Show what would be done without making changes
- `-m, --message`: Custom commit message
- `-t, --tag-only`: Only create git tag, don't update files
- `-f, --force`: Force update even if working directory is not clean

**Examples:**
```bash
# Patch release
./scripts/update-version.sh 1.0.1

# Minor release
./scripts/update-version.sh 1.1.0

# Major release
./scripts/update-version.sh 2.0.0

# Pre-release
./scripts/update-version.sh 1.0.0-alpha.1

# Release candidate
./scripts/update-version.sh 1.0.0-rc.1
```

## Requirements

- Bash shell
- Git repository
- Write access to remote repository
- `sed`, `awk`, `date` commands available

## Safety Features

- **Version validation**: Ensures semantic versioning format
- **Git status check**: Prevents updates with uncommitted changes
- **Dry-run mode**: Test changes before applying
- **Backup creation**: Creates backup files during updates
- **Error handling**: Exits on any error with informative messages

## Workflow

1. **Development**: Make changes to the codebase
2. **Testing**: Test the changes thoroughly
3. **Version update**: Run the script with appropriate version
4. **Review**: Check the generated commit and tag
5. **Release**: Create GitHub release if needed

## Integration

This script is designed to integrate with:
- OpenWRT build system
- GitHub releases
- CI/CD pipelines
- Automated testing workflows 