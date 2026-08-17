# Presets

## Directory Structure

```
presets/
├── factory/        # Factory presets (read-only, bundled with app)
│   ├── 01_clean_sparkle.preset
│   ├── 02_jazz_box.preset
│   └── ... (35 total)
├── user/           # User-created presets
└── FACTORY_PRESETS.md  # Documentation of all factory presets
```

## Preset File Format

Presets are stored as simple key-value text files:

```
name=Clean Sparkle
inputGainDb=-6.0
ampDrive=0.35
reverbRoom=0.35
...
```

## Loading Presets

### iOS (Swift)
```swift
let preset = try PresetStore.loadPreset(from: path)
audioEngine.applyPreset(preset)
```

### Android (Kotlin)
```kotlin
val preset = presetStore.loadPreset(path)
audioEngine.applyPreset(preset)
```

## Factory Preset Categories

1. **Clean Tones** (01-04): Sparkle, Jazz, Glassy, Acoustic Sim
2. **Crunch Tones** (05-08): Edge, Blues, Vintage, Plexi
3. **High Gain** (09-12): Heavy Lead, Modern, Classic Metal, Brown Sound
4. **Ambient** (13-16): Clean, Shimmer, Spacey, Swell
5. **Fuzz** (17-20): Stomp, Octave, Tone Bender, Velcro
6. **Effects** (21-26): Chorus, Flanger, Phaser, Tremolo, Wah
7. **Utility** (27-30): Quiet, Looper, Recording, Live
8. **Hybrid** (31-32): Piezo Sim, Acoustic-Electric
9. **Bass** (33-35): Clean, Overdrive, Synth
