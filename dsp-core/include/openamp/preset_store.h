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
    std::string cabIrPath;
};

class PresetStore {
public:
    static bool savePreset(const Preset& preset, const std::string& path, std::string& error);
    static bool loadPreset(const std::string& path, Preset& preset, std::string& error);
};

} // namespace openamp
