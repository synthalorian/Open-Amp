#pragma once

#include "openamp/amp_simulator.h"
#include "openamp/effect_chain.h"
#include "noise_gate.h"
#include "compressor.h"
#include "eq.h"
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
    
    // Phase 2: Noise Gate / Compressor / EQ controls
    void setNoiseGateEnabled(bool enabled) { noiseGateEnabled_ = enabled; }
    void setCompressorEnabled(bool enabled) { compressorEnabled_ = enabled; }
    void setEQEnabled(bool enabled) { eqEnabled_ = enabled; }
    bool isNoiseGateEnabled() const { return noiseGateEnabled_; }
    bool isCompressorEnabled() const { return compressorEnabled_; }
    bool isEQEnabled() const { return eqEnabled_; }
    
    NoiseGate* getNoiseGate() { return noiseGate_.get(); }
    Compressor* getCompressor() { return compressor_.get(); }
    EQ* getEQ() { return eq_.get(); }
    
    float getInputLevel() const { return inputLevel_.load(); }
    float getOutputLevel() const { return outputLevel_.load(); }
    
    bool isClipping() const { return clipping_.load(); }
    void resetClipIndicator();
    
    const ProcessingConfig& getConfig() const { return config_; }
    
private:
    ProcessingConfig config_;
    
    std::unique_ptr<AmpSimulator> ampSimulator_;
    std::unique_ptr<EffectChain> effectChain_;
    
    // Phase 2: Input dynamics and tone shaping
    std::unique_ptr<NoiseGate> noiseGate_;
    std::unique_ptr<Compressor> compressor_;
    std::unique_ptr<EQ> eq_;
    
    float inputGain_ = 1.0f;
    float outputGain_ = 1.0f;
    float inputGainDb_ = 0.0f;
    float outputGainDb_ = 0.0f;
    bool ampEnabled_ = true;
    bool effectsEnabled_ = true;
    bool noiseGateEnabled_ = true;
    bool compressorEnabled_ = false;
    bool eqEnabled_ = false;
    
    std::atomic<float> inputLevel_{0.0f};
    std::atomic<float> outputLevel_{0.0f};
    std::atomic<bool> clipping_{false};
    
    std::mutex processingMutex_;
    
    void calculateLevels(const float* buffer, uint32_t numFrames, std::atomic<float>& level);
};

} // namespace openamp
