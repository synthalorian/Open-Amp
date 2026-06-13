#include "openamp/input_processor.h"
#include "openamp/dsp_utils.h"
#include "openamp/preset_store.h"
#include "noise_gate.h"
#include "compressor.h"
#include "eq.h"
#include <cmath>
#include <vector>
#include <cstdlib>

namespace openamp {

InputProcessor::InputProcessor() = default;
InputProcessor::~InputProcessor() = default;

bool InputProcessor::initialize(const ProcessingConfig& config) {
    config_ = config;
    
    if (ampSimulator_) {
        ampSimulator_->prepare(config.sampleRate, config.bufferSize);
    }
    
    if (effectChain_) {
        effectChain_->prepare(config.sampleRate, config.bufferSize);
    }
    
    // Phase 2: Initialize noise gate, compressor, EQ
    if (!noiseGate_) {
        noiseGate_ = std::make_unique<NoiseGate>();
        noiseGate_->prepare(config.sampleRate, config.bufferSize);
        noiseGate_->setThreshold(-45.0f);
        noiseGate_->setAttack(1.0f);
        noiseGate_->setRelease(100.0f);
    }
    if (!compressor_) {
        compressor_ = std::make_unique<Compressor>();
        compressor_->prepare(config.sampleRate, config.bufferSize);
        compressor_->setThreshold(-20.0f);
        compressor_->setRatio(4.0f);
        compressor_->setAttack(10.0f);
        compressor_->setRelease(100.0f);
    }
    if (!eq_) {
        eq_ = std::make_unique<EQ>();
        eq_->prepare(config.sampleRate, config.bufferSize);
    }
    
    return true;
}

void InputProcessor::shutdown() {
    ampSimulator_.reset();
    effectChain_.reset();
    noiseGate_.reset();
    compressor_.reset();
    eq_.reset();
}

void InputProcessor::processInput(const float* inputBuffer, float* outputBuffer, uint32_t numFrames) {
    if (!inputBuffer || !outputBuffer) return;
    
    // Ensure scratch buffer is large enough (pre-allocated, no RT allocation)
    if (scratchBuffer_.size() < numFrames) {
        scratchBuffer_.resize(numFrames);
    }
    
    calculateLevels(inputBuffer, numFrames, inputLevel_);
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        scratchBuffer_[i] = inputBuffer[i] * inputGain_;
    }
    
    // Phase 2: Signal chain — Noise Gate → Compressor → EQ → Effects → Amp
    AudioBuffer monoBuffer;
    monoBuffer.data = scratchBuffer_.data();
    monoBuffer.numChannels = 1;
    monoBuffer.numFrames = numFrames;
    monoBuffer.sampleRate = static_cast<uint32_t>(config_.sampleRate);
    
    if (noiseGate_ && noiseGateEnabled_.load(std::memory_order_relaxed)) {
        noiseGate_->process(monoBuffer);
    }
    
    if (compressor_ && compressorEnabled_.load(std::memory_order_relaxed)) {
        compressor_->process(monoBuffer);
    }
    
    if (eq_ && eqEnabled_.load(std::memory_order_relaxed)) {
        eq_->process(monoBuffer);
    }
    
    if (effectChain_ && effectsEnabled_.load(std::memory_order_relaxed)) {
        effectChain_->process(monoBuffer);
    }
    
    if (ampSimulator_ && ampEnabled_.load(std::memory_order_relaxed)) {
        ampSimulator_->process(monoBuffer);
    }
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = scratchBuffer_[i] * outputGain_;
        
        if (std::abs(sample) >= 0.99f) {
            clipping_.store(true);
        }
        
        for (uint32_t ch = 0; ch < config_.numOutputChannels; ++ch) {
            outputBuffer[i * config_.numOutputChannels + ch] = sample;
        }
    }
    
    calculateLevels(outputBuffer, numFrames * config_.numOutputChannels, outputLevel_);
}

void InputProcessor::setAmpSimulator(std::unique_ptr<AmpSimulator> amp) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    ampSimulator_ = std::move(amp);
    if (ampSimulator_) {
        ampSimulator_->prepare(config_.sampleRate, config_.bufferSize);
    }
}

void InputProcessor::setEffectChain(std::unique_ptr<EffectChain> effects) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    effectChain_ = std::move(effects);
    if (effectChain_) {
        effectChain_->prepare(config_.sampleRate, config_.bufferSize);
    }
}

void InputProcessor::setInputGain(float gainDb) {
    inputGainDb_ = gainDb;
    inputGain_ = DSPUtils::dbToGain(gainDb);
}

void InputProcessor::setOutputGain(float gainDb) {
    outputGainDb_ = gainDb;
    outputGain_ = DSPUtils::dbToGain(gainDb);
}

void InputProcessor::setAmpEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    ampEnabled_ = enabled;
}

void InputProcessor::setEffectsEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    effectsEnabled_ = enabled;
}

void InputProcessor::applyPreset(const Preset& preset) {
    setInputGain(preset.inputGainDb);
    setOutputGain(preset.outputGainDb);
    setAmpEnabled(preset.ampEnabled);
    setEffectsEnabled(preset.effectsEnabled);
}

Preset InputProcessor::capturePreset(const std::string& name) const {
    Preset preset;
    preset.name = name;
    preset.inputGainDb = inputGainDb_;
    preset.outputGainDb = outputGainDb_;
    preset.ampEnabled = ampEnabled_;
    preset.effectsEnabled = effectsEnabled_;
    return preset;
}

void InputProcessor::resetClipIndicator() {
    clipping_.store(false);
}

void InputProcessor::calculateLevels(const float* buffer, uint32_t numFrames, std::atomic<float>& level) {
    float sum = 0.0f;
    for (uint32_t i = 0; i < numFrames; ++i) {
        sum += buffer[i] * buffer[i];
    }
    float rms = std::sqrt(sum / numFrames);
    level.store(rms);
}

} // namespace openamp
