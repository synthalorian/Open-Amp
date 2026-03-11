# Guitar Amp - Linux

A professional guitar amplifier and effects processor for Linux.

## Features

- **Real-time DSP processing** - Low-latency audio processing
- **Amp simulation** - Tube amp modeling with full EQ control
- **Effects** - Distortion, Delay, Reverb with pedal-style controls
- **75 Factory presets** - Covering all genres and playing styles
- **MIDI support** - Control parameters via MIDI controllers
- **PipeWire & ALSA** - Modern and legacy audio backends

## Requirements

### Required
- **Qt 6** - Core, Widgets, Quick, QuickControls2
- **CMake** 3.20+
- **C++17** compiler (GCC 9+ or Clang 10+)

### Audio (at least one)
- **PipeWire** 0.3+ (recommended)
- **ALSA** (libasound)

### Optional
- **ALSA MIDI** - For MIDI controller support

## Installation

### Arch Linux
```bash
sudo pacman -S qt6-base qt6-declarative pipewire cmake gcc
```

### Ubuntu/Debian
```bash
sudo apt install qt6-base-dev qt6-declarative-dev \
                 libpipewire-0.3-dev libasound2-dev \
                 cmake g++
```

### Fedora
```bash
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel \
                 pipewire-devel alsa-lib-devel cmake gcc-c++
```

## Building

```bash
cd linux
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `USE_PIPEWIRE` | ON | Enable PipeWire audio backend |
| `USE_ALSA` | ON | Enable ALSA audio backend |
| `BUILD_TESTS` | OFF | Build unit tests |

Example:
```bash
cmake -DUSE_PIPEWIRE=ON -DUSE_ALSA=ON ..
```

## Running

```bash
./guitar-amp-linux
```

For low-latency performance, consider using a realtime kernel or setting up
PipeWire with realtime priorities.

## Audio Setup

### PipeWire (Recommended)
PipeWire provides the best low-latency experience. Most modern distributions
have it installed by default.

Check PipeWire is running:
```bash
pw-cli info 0
```

### ALSA
If PipeWire is not available, the app will fall back to ALSA. For low latency:
```bash
# Set real-time priority for audio group
sudo usermod -a -G audio $USER
```

### Latency Tips
- Use smaller buffer sizes (64-256 samples)
- Disable CPU frequency scaling
- Use a realtime or low-latency kernel
- Close other audio applications

## MIDI Control

The app creates a MIDI port that can be connected to external controllers:

```bash
# List MIDI ports
aconnect -l

# Connect a controller
aconnect <controller> <GuitarAmp>
```

### Default CC Mappings

| CC | Parameter |
|----|-----------|
| 1 | Modulation (future) |
| 7 | Master Volume |
| 11 | Expression |
| 20 | Drive |
| 21 | Delay Mix |
| 22 | Reverb Mix |

## Configuration

Settings are stored in:
```
~/.config/Synthalorian/GuitarAmp.conf
```

User presets are stored in:
```
~/.local/share/Synthalorian/GuitarAmp/presets/
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+O` | Open preset |
| `Ctrl+S` | Save preset |
| `Ctrl+,` | Audio settings |
| `Ctrl+Q` | Quit |
| `Space` | Toggle audio |

## Troubleshooting

### No audio
1. Check audio backend in Settings menu
2. Verify input/output device selection
3. Check PipeWire/ALSA configuration

### High latency
1. Reduce buffer size in Settings
2. Use PipeWire instead of ALSA
3. Set up realtime audio priorities

### Build errors
1. Ensure all Qt6 packages are installed
2. Check CMake version (3.20+)
3. Verify compiler supports C++17

## License

MIT License - See LICENSE file for details.

## Credits

Developed by Synthalorian

Built with:
- Qt 6
- PipeWire / ALSA
- DSP algorithms from the guitar-amp-app core
