#include "openamp/preset_store.h"
#include <fstream>
#include <sstream>

namespace openamp {

static bool parseBool(const std::string& value) {
    return value == "1" || value == "true" || value == "True";
}

bool PresetStore::savePreset(const Preset& preset, const std::string& path, std::string& error) {
    error.clear();
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        error = "Failed to open preset file for writing.";
        return false;
    }

    file << "name=" << preset.name << "\n";
    file << "inputGainDb=" << preset.inputGainDb << "\n";
    file << "outputGainDb=" << preset.outputGainDb << "\n";
    file << "ampEnabled=" << (preset.ampEnabled ? "1" : "0") << "\n";
    file << "effectsEnabled=" << (preset.effectsEnabled ? "1" : "0") << "\n";
    file << "delayEnabled=" << (preset.delayEnabled ? "1" : "0") << "\n";
    file << "reverbEnabled=" << (preset.reverbEnabled ? "1" : "0") << "\n";
    file << "distortionEnabled=" << (preset.distortionEnabled ? "1" : "0") << "\n";
    file << "delayFirst=" << (preset.delayFirst ? "1" : "0") << "\n";
    file << "delayTimeMs=" << preset.delayTimeMs << "\n";
    file << "delayFeedback=" << preset.delayFeedback << "\n";
    file << "delayMix=" << preset.delayMix << "\n";
    file << "reverbRoom=" << preset.reverbRoom << "\n";
    file << "reverbDamp=" << preset.reverbDamp << "\n";
    file << "reverbMix=" << preset.reverbMix << "\n";
    file << "distortionDrive=" << preset.distortionDrive << "\n";
    file << "distortionTone=" << preset.distortionTone << "\n";
    file << "distortionLevel=" << preset.distortionLevel << "\n";
    file << "distortionType=" << preset.distortionType << "\n";
    file << "ampGainDb=" << preset.ampGainDb << "\n";
    file << "ampDrive=" << preset.ampDrive << "\n";
    file << "ampBassDb=" << preset.ampBassDb << "\n";
    file << "ampMidDb=" << preset.ampMidDb << "\n";
    file << "ampTrebleDb=" << preset.ampTrebleDb << "\n";
    file << "ampPresenceDb=" << preset.ampPresenceDb << "\n";
    file << "ampMasterDb=" << preset.ampMasterDb << "\n";
    file << "cabIrPath=" << preset.cabIrPath << "\n";
    return true;
}

bool PresetStore::loadPreset(const std::string& path, Preset& preset, std::string& error) {
    error.clear();
    std::ifstream file(path);
    if (!file.is_open()) {
        error = "Failed to open preset file for reading.";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "name") preset.name = value;
        else if (key == "inputGainDb") preset.inputGainDb = std::stof(value);
        else if (key == "outputGainDb") preset.outputGainDb = std::stof(value);
        else if (key == "ampEnabled") preset.ampEnabled = parseBool(value);
        else if (key == "effectsEnabled") preset.effectsEnabled = parseBool(value);
        else if (key == "delayEnabled") preset.delayEnabled = parseBool(value);
        else if (key == "reverbEnabled") preset.reverbEnabled = parseBool(value);
        else if (key == "distortionEnabled") preset.distortionEnabled = parseBool(value);
        else if (key == "delayFirst") preset.delayFirst = parseBool(value);
        else if (key == "delayTimeMs") preset.delayTimeMs = std::stof(value);
        else if (key == "delayFeedback") preset.delayFeedback = std::stof(value);
        else if (key == "delayMix") preset.delayMix = std::stof(value);
        else if (key == "reverbRoom") preset.reverbRoom = std::stof(value);
        else if (key == "reverbDamp") preset.reverbDamp = std::stof(value);
        else if (key == "reverbMix") preset.reverbMix = std::stof(value);
        else if (key == "distortionDrive") preset.distortionDrive = std::stof(value);
        else if (key == "distortionTone") preset.distortionTone = std::stof(value);
        else if (key == "distortionLevel") preset.distortionLevel = std::stof(value);
        else if (key == "distortionType") preset.distortionType = std::stoi(value);
        else if (key == "ampGainDb") preset.ampGainDb = std::stof(value);
        else if (key == "ampDrive") preset.ampDrive = std::stof(value);
        else if (key == "ampBassDb") preset.ampBassDb = std::stof(value);
        else if (key == "ampMidDb") preset.ampMidDb = std::stof(value);
        else if (key == "ampTrebleDb") preset.ampTrebleDb = std::stof(value);
        else if (key == "ampPresenceDb") preset.ampPresenceDb = std::stof(value);
        else if (key == "ampMasterDb") preset.ampMasterDb = std::stof(value);
        else if (key == "cabIrPath") preset.cabIrPath = value;
    }

    return true;
}

} // namespace openamp
