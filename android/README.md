# Android Audio I/O Wrapper

This folder contains the Android native audio wrapper built on Oboe and the shared DSP core.

## Structure

- `app/src/main/cpp` JNI + native audio engine
- `app/src/main/java` Kotlin wrapper

## Notes

- The native layer expects Oboe to be available in the CMake build.
- For Android Studio, add Oboe as a dependency and pass `OBOE_DIR` to CMake.
- The DSP core is linked in via `dsp-core/src` and `dsp-core/include`.

## Configure Oboe

Add `OBOE_DIR` to `android/gradle.properties` (local override) pointing to your Oboe checkout:

```
OBOE_DIR=/absolute/path/to/oboe
```

## USB device selection

The app lists available USB audio devices and lets you pick input/output routing. When you switch devices while running, the engine restarts to reopen streams with the selected device IDs.

## Presets

Presets are saved to the app files directory as simple `.preset` files. Use the Preset Name field with Save/Load to store or recall settings.
