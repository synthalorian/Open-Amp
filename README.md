# Open Amp

A professional cross-platform guitar amplifier and effects processor.

## Features

- **Real-time DSP processing** - Low-latency audio processing
- **Amp simulation** - Tube amp modeling with full EQ control
- **12 Effect plugins** - Distortion, Delay, Reverb, EQ, Compressor, Modulation, Noise Gate, Wah, Tuner, Looper, Metronome, Recorder
- **100 Factory presets** - Covering all genres and playing styles
- **MIDI support** - Control parameters via MIDI controllers
- **Cross-platform** - Linux, iOS, and Android

## 📱 Platforms
- **Linux** - Qt 6 UI with PipeWire/ALSA audio
- **Android** - Kotlin UI with Oboe audio engine
- **iOS** - SwiftUI interface with AVAudioEngine

## Project Structure

```
openamp/
├── dsp-core/                    # Shared C++ DSP engine
│   ├── include/openamp/          # Public headers
│   │   ├── amp_simulator.h
│   │   ├── effect_chain.h
│   │   ├── plugin_interface.h
│   │   ├── preset_store.h
│   │   ├── input_processor.h
│   │   └── dsp_utils.h
│   ├── src/                     # Core implementation
│   └── plugins/                 # Effect plugins (12 total)
│       ├── distortion/          # 4 types: Overdrive, Fuzz, Tube, HardClip
│       ├── delay/               # Normal, PingPong, Slapback, Tape modes
│       ├── reverb/              # Hall, Room, Plate, Spring types
│       ├── eq/                  # 10-band parametric EQ
│       ├── compressor/          # Studio compressor
│       ├── modulation/          # Chorus, Flanger, Phaser, Tremolo, Vibrato
│       ├── noise_gate/          # Intelligent gate
│       ├── wah/                 # Auto-wah / envelope filter
│       ├── tuner/               # Chromatic tuner
│       ├── looper/              # Loop station
│       ├── metronome/           # Click track
│       └── recorder/            # Audio recording
│
├── linux/                       # Linux app
│   └── src/
│       ├── audio/               # PipeWire & ALSA backends
│       ├── ui/                  # Qt 6 widgets
│       │   ├── main_window.cpp
│       │   ├── knob_widget.cpp
│       │   ├── pedal_widget.cpp
│       │   ├── meter_widget.cpp
│       │   ├── preset_browser.cpp
│       │   └── ...
│       └── main.cpp
│
├── android/                     # Android app
│   └── app/src/main/
│       ├── cpp/                 # JNI bridge + audio engine
│       ├── java/                # Kotlin UI
│       └── assets/presets/      # 100 factory presets
│
├── ios/                         # iOS app
│   └── Open Amp/
│       ├── AudioEngine.swift    # Swift audio engine
│       ├── OpenAmpApp.swift      # App entry point
│       ├── Resources/Presets/   # 100 factory presets
│       └── Views/               # SwiftUI views
│
└── presets/
    └── factory/                 # 100 presets (.preset files)
```

## Presets (100 Total)

### Clean & Vintage (1-4, 36-40, 52, 61-69, 86-89, 92)
- Clean Sparkle, Jazz Box, Glassy Shimmer, Acoustic Sim
- Surf Wash, Twangy Tele, Dark Jazz, Funk Slap, Motown Clean
- Neo Soul Clean, Spank Plank, Warm Fat Clean, Edge Breakup
- Comp Studio, Classical Nylon, Flat Response, Pedal Platform
- Small Combo, Headroom Max, Acoustic Rock, Singer Songwriter
- Fingerpicking, Studio Clean

### Rock (5-8, 49-51, 53-54, 63, 70-73, 91, 97-98)
- Edge Crunch, Blues Driver, Vintage Crunch, Plexi Crunch
- Chicago Blues, Delta Bottleneck, Country Chicken, R&B Comp
- Slash Lead, Edge Breakup, Brit Pop Crunch, Indie Jangle
- Alt 90s Crunch, Garage Raw, Christian Rock, Jam Track, Live Loud

### High Gain (9-12, 41-43, 55-59, 76-79)
- Heavy Lead, Modern High Gain, Classic Metal, Brown Sound
- Punk Power, Grunge Sludge, Stoner Doom
- Metalcore Rhythm, Progressive Djent, Blackened Crunch
- Thrash Attack, Glam Arena
- Metal Rhythm, Metal Lead, Drop Tuned, Neoclassical

### Ambient & Atmospheric (13-16, 44-45, 74-75, 80-82)
- Ambient Clean, Shimmer Pad, Spacey Echo, Swell Builder
- Post Rock, Shoegaze Wall, Psych Reverse, Tape Wobble
- Synth Lead, Ambient Pad, Ethereal

### Fuzz & Specialty (17-20, 83-85)
- Fuzz Stomp, Octave Fuzz, Tone Bender, Velcro Fuzz
- Glitch Stutter, LoFi Grit, Bit Crusher

### Effects Showcase (21-26)
- Chorus Dreams, Flanger Jet, Phaser Space, Tremolo Vintage
- Wah Funk, Wah Cry

### Utility (27-30, 64-69, 93-96, 99-100)
- Quiet Practice, Looper Setup, Recording Direct, Live Stage
- DI Recording, Reamping Prep, Practice Quiet, Headphones Late
- Festival Main, Stadium Huge

### Bass (31-35)
- Bass Clean, Bass Overdrive, Bass Synth

## Building

### Linux
```bash
cd linux
./build.sh
```

Or manually:
```bash
cd linux
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

**Requirements:**
- Qt 6 (Core, Widgets, Quick, QuickControls2)
- PipeWire or ALSA
- CMake 3.20+
- C++17 compiler

### Android
1. Open `android/` in Android Studio
2. Sync Gradle files
3. Build and run on device

### iOS
1. Open `ios/Open Amp.xcodeproj` in Xcode
2. Select your target device
3. Build and run

## Audio Specifications

- **Sample Rate:** 44.1kHz - 192kHz (48kHz recommended)
- **Buffer Size:** 64 - 4096 samples (256 recommended for low latency)
- **Latency:** ~5ms at 48kHz/256 samples
- **Bit Depth:** 32-bit float internal processing

## Effect Algorithms

### Distortion (v1.1)
- **Overdrive:** Asymmetrical soft clipping (Tube Screamer style)
- **Fuzz:** Octave fuzz with transistor clipping
- **Tube:** Realistic tube saturation with positive/negative asymmetry
- **Hard Clip:** Aggressive digital clipping

### Delay (v1.1)
- **Normal:** Classic digital delay
- **PingPong:** Stereo bouncing delay
- **Slapback:** Quick echo with no feedback
- **Tape:** Wow/flutter modulation with LP filtering

### Reverb (v1.1)
- **Hall:** Large concert hall
- **Room:** Small to medium room
- **Plate:** Classic plate reverb
- **Spring:** Vintage spring tank simulation
- Pre-delay and width controls

## MIDI Control

The app responds to standard MIDI CC messages:

| CC | Parameter |
|----|-----------|
| 7 | Master Volume |
| 11 | Expression |
| 20 | Drive |
| 21 | Delay Mix |
| 22 | Reverb Mix |
| 23 | Delay Time |
| 24 | Reverb Room |

## License

MIT License - See LICENSE file for details.

## Credits

Developed by Synthalorian

Built with:
- Qt 6 (Linux)
- AVAudioEngine (iOS)
- Oboe (Android)
- Custom DSP algorithms
