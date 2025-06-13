# Bisect

A command-line utility for efficiently searching timestamps in large log files using binary search algorithms.

## Overview

Bisect helps you quickly find specific timestamps in chronologically ordered log files without having to scan the entire file. It uses binary search to locate timestamps, making it particularly useful for analyzing large log files.

## Features

- **Binary search** for efficient timestamp location in large files
- **Flexible time range parsing** with offset modifiers (+, -, ~)
- **Multiple time units** supported (seconds, minutes, hours, days)
- **Verbose output** for debugging and detailed information
- **Robust error handling** with clear error messages

## Installation

### Build from Source

```bash
make
```

### Install System-wide

```bash
make install
```

This will copy the binary to `/usr/local/bin/bisect`.

## Usage

```bash
bisect [OPTIONS] <filename>
```

### Arguments

- `filename` - Input log file to process (must be chronologically ordered)

### Options

- `-h, --help` - Show help message
- `-v, --version` - Show version information  
- `-t, --time TIME` - Target time to search for (required)
- `-V, --verbose` - Enable verbose output

### Time Format

The basic time format is: `YYYY-MM-DD HH:MM:SS`

#### Time Range Modifiers

You can specify time ranges using these modifiers:

- `+Xu` - Search forward from time by X units
- `-Xu` - Search backward from time by X units  
- `~Xu` - Search both directions (±X units from time)

Where `u` is one of:
- `s` - seconds
- `m` - minutes
- `h` - hours
- `d` - days

#### Examples

```bash
# Find exact timestamp
bisect -t "2025-06-02 11:55:34" application.log

# Find entries within 30 seconds after timestamp
bisect -t "2025-06-02 11:55:34+30s" application.log

# Find entries from 1 hour before timestamp
bisect -t "2025-06-02 11:55:34-1h" application.log

# Find entries within 2 hours before and after timestamp
bisect -t "2025-06-02 11:55:34~2h" application.log

# Verbose output
bisect -V -t "2025-06-02 11:55:34" application.log
```

## File Requirements

- Log files must contain timestamps in `YYYY-MM-DD HH:MM:SS` format
- Timestamps must be in chronological order
- Files must be readable by the user

## Development

### Building

```bash
make              # Build release version
make debug        # Build debug version with symbols
make test         # Build and run tests
make clean        # Clean build artifacts
```

### Testing

The project includes comprehensive unit tests:

```bash
make test
```

Tests cover:
- Time string parsing and validation
- Range calculation with various modifiers
- Error handling for invalid inputs
- Edge cases and boundary conditions

### Project Structure

- `main.c` - Command-line interface and argument parsing
- `bisect_lib.c` - Core binary search and file processing logic
- `search_range.c` - Time range parsing and validation
- `test.c` - Unit tests
- `*.h` - Header files with function declarations

## License

This project is open source.