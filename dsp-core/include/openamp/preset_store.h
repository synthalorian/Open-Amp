#pragma once

#include <string>

namespace openamp {

struct Preset {
    std::string name;
    float inputGainDb = 0.0f;
    float outputGainDb = 0.0f;
    bool ampEnabled = true;
    bool effectsEnabled = true;
    bool delayEnabled = true;
    bool reverbEnabled = true;
    bool distortionEnabled = false;
    bool delayFirst = true;
    float delayTimeMs = 350.0f;
    float delayFeedback = 0.35f;
    float delayMix = 0.25f;
    float reverbRoom = 0.5f;
    float reverbDamp = 0.3f;
    float reverbMix = 0.25f;
    float distortionDrive = 0.5f;
    float distortionTone = 0.5f;
    float distortionLevel = 0.7f;
    int distortionType = 0;
    float ampGainDb = 0.0f;
    float ampDrive = 0.5f;
    float ampBassDb = 0.0f;
    float ampMidDb = 0.0f;
    float ampTrebleDb = 0.0f;
    float ampPresenceDb = 0.0f;
    float ampMasterDb = 0.0f;
    bool noiseGateEnabled = true;
    float noiseGateThreshold = -45.0f;
    float noiseGateAttack = 1.0f;
    float noiseGateRelease = 100.0f;
    std::string cabIrPath;

    // Phase 4: Modulation
    bool modulationEnabled = false;
    int modulationType = 0;        // Chorus=0, Flanger=1, Phaser=2, Tremolo=3, Vibrato=4
    float modulationRate = 1.5f;
    float modulationDepth = 0.5f;
    float modulationMix = 0.5f;

    // Phase 4: Cabinet
    bool cabinetEnabled = true;
    int cabinetType = 0;           // Marshall4x12=0, Fender2x12=1, etc.
    float cabinetMix = 1.0f;

    // Phase 4: Acoustic Sim
    bool acousticSimEnabled = false;
    float acousticAmount = 0.5f;
    float acousticBodySize = 0.5f;
    float acousticBrightness = 0.5f;

    // Phase 4: Harmonizer
    bool harmonizerEnabled = false;
    int harmonizerMode = 0;        // OctaveUp=0, OctaveDown=1, PerfectFifth=2, etc.
    float harmonizerMix = 0.5f;
};

class PresetStore {
public:
    static bool savePreset(const Preset& preset, const std::string& path, std::string& error);
    static bool loadPreset(const std::string& path, Preset& preset, std::string& error);
};

} // namespace openamp
