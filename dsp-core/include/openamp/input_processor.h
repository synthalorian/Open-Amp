#pragma once

#include "openamp/amp_simulator.h"
#include "openamp/effect_chain.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace openamp {

struct Preset;

enum class InputSource {
    USB,
    BuiltInMic,
    Bluetooth
};

struct ProcessingConfig {
    double sampleRate = 48000.0;
    uint32_t bufferSize = 128;
    uint32_t numInputChannels = 1;
    uint32_t numOutputChannels = 2;
    InputSource inputSource = InputSource::USB;
    bool enableMonitoring = true;
};

class InputProcessor {
public:
    InputProcessor();
    ~InputProcessor();
    
    bool initialize(const ProcessingConfig& config);
    void shutdown();
    
    void processInput(const float* inputBuffer, float* outputBuffer, uint32_t numFrames);
    
    void setAmpSimulator(std::unique_ptr<AmpSimulator> amp);
    void setEffectChain(std::unique_ptr<EffectChain> effects);
    
    void setInputGain(float gainDb);
    void setOutputGain(float gainDb);
    float getInputGainDb() const { return inputGainDb_; }
    float getOutputGainDb() const { return outputGainDb_; }
    
    void applyPreset(const Preset& preset);
    Preset capturePreset(const std::string& name) const;
    
    void setAmpEnabled(bool enabled);
    void setEffectsEnabled(bool enabled);
    bool isAmpEnabled() const { return ampEnabled_; }
    bool isEffectsEnabled() const { return effectsEnabled_; }
    
    float getInputLevel() const { return inputLevel_.load(); }
    float getOutputLevel() const { return outputLevel_.load(); }
    
    bool isClipping() const { return clipping_.load(); }
    void resetClipIndicator();
    
    const ProcessingConfig& getConfig() const { return config_; }
    
private:
    ProcessingConfig config_;
    
    std::unique_ptr<AmpSimulator> ampSimulator_;
    std::unique_ptr<EffectChain> effectChain_;
    
    float inputGain_ = 1.0f;
    float outputGain_ = 1.0f;
    float inputGainDb_ = 0.0f;
    float outputGainDb_ = 0.0f;
    bool ampEnabled_ = true;
    bool effectsEnabled_ = true;
    
    std::atomic<float> inputLevel_{0.0f};
    std::atomic<float> outputLevel_{0.0f};
    std::atomic<bool> clipping_{false};
    
    std::mutex processingMutex_;
    
    void calculateLevels(const float* buffer, uint32_t numFrames, std::atomic<float>& level);
};

} // namespace openamp
