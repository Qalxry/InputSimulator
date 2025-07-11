# Input Simulator

A Windows command-line utility for simulating mouse clicks, keyboard presses, and cursor movements with advanced features like smooth movement, DPI awareness, and focus switching.

## Features

- **Mouse Operations**: Left/right/middle click, double-click, wheel scrolling, cursor movement
- **Keyboard Operations**: Press any key with support for function keys, control keys, and alphanumeric keys
- **Smooth Movement**: Linear and eased cursor movement with customizable duration
- **DPI Awareness**: Automatic DPI scaling detection and handling
- **Focus Management**: Temporary focus switching capability
- **Batch Processing**: Execute multiple commands from a file
- **Flexible Modes**: Return cursor to original position after actions
- **Consistent Coordinates**: Option to ignore external mouse movement during operations, which is useful when using batch processing.

## Usage

```bash
input_simulator.exe [OPTIONS]
```

### Basic Examples

```bash
# Move mouse to coordinates
input_simulator.exe -k mouse_move -x 500 -y 500

# Left click at specific position
input_simulator.exe -k mouse_left -x 500 -y 500

# Press Enter key
input_simulator.exe -k key_enter -a click

# Smooth movement with easing
input_simulator.exe -k mouse_left -x 800 -y 600 -sm ease -smt 300

# Return to original position after click
input_simulator.exe -k mouse_right -x 800 -y 600 -m back
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-k, --key` | Input type (mouse_left, mouse_right, key_enter, etc.) |
| `-a, --action` | Action (click, doubleclick, keydown, keyup) |
| `-x, -y` | Target coordinates (-1 to keep current position) |
| `-m, --mode` | Mode (none, back) - return to original position |
| `-sm, --smooth` | Smooth movement (none, linear, ease) |
| `-smt, --smooth_time` | Movement duration in milliseconds |
| `-s, --sleep` | Sleep time after action |
| `-f, --file` | Execute commands from file |
| `-c, --consistent` | Ignore external mouse movement |
| `-v, --verbose` | Verbose output |
| `-h, --help` | Show help |

### Batch Processing

Create a text file with commands (one per line):

```plaintext
-k mouse_left -x 100 -y 100
-k key_enter -a click
-s 1000
```

Execute with: `input_simulator.exe -f commands.txt`

## Build

### CMake

To build the Input Simulator, you can use CMake. Follow these steps:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Manual Compilation

Ensure you have a C++20 compatible compiler (like g++) and the necessary libraries installed.

```bash
g++ -std=c++20 -o input_simulator.exe main.cpp -luser32 -lshcore
```

## License

MIT License
