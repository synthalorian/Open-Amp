#include "openamp/input_processor.h"
#include "openamp/dsp_utils.h"
#include "openamp/preset_store.h"
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
    
    return true;
}

void InputProcessor::shutdown() {
    ampSimulator_.reset();
    effectChain_.reset();
}

void InputProcessor::processInput(const float* inputBuffer, float* outputBuffer, uint32_t numFrames) {
    if (!inputBuffer || !outputBuffer) return;
    
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    calculateLevels(inputBuffer, numFrames, inputLevel_);
    
    std::vector<float> processedBuffer(numFrames);
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        processedBuffer[i] = inputBuffer[i] * inputGain_;
    }
    
    if (effectChain_ && effectsEnabled_) {
        AudioBuffer effectBuffer;
        effectBuffer.data = processedBuffer.data();
        effectBuffer.numChannels = 1;
        effectBuffer.numFrames = numFrames;
        effectBuffer.sampleRate = static_cast<uint32_t>(config_.sampleRate);
        
        effectChain_->process(effectBuffer);
    }
    
    if (ampSimulator_ && ampEnabled_) {
        AudioBuffer ampBuffer;
        ampBuffer.data = processedBuffer.data();
        ampBuffer.numChannels = 1;
        ampBuffer.numFrames = numFrames;
        ampBuffer.sampleRate = static_cast<uint32_t>(config_.sampleRate);
        
        ampSimulator_->process(ampBuffer);
    }
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = processedBuffer[i] * outputGain_;

        // Add subtle amplifier hiss when amp is enabled (authentic analog feel)
        if (ampEnabled_) {
            float noise = ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * 0.0008f;
            sample += noise;
        }
        
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
