#include "noise_gate.h"
#include <cmath>
#include <algorithm>

namespace openamp {

NoiseGate::NoiseGate() {
    updateCoefficients();
}

void NoiseGate::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    updateCoefficients();
    reset();
}

void NoiseGate::updateCoefficients() {
    float attackSamples = (attackMs_ / 1000.0f) * static_cast<float>(sampleRate_);
    float releaseSamples = (releaseMs_ / 1000.0f) * static_cast<float>(sampleRate_);
    
    attackCoeff_ = std::exp(-1.0f / attackSamples);
    releaseCoeff_ = std::exp(-1.0f / releaseSamples);
    holdSamples_ = static_cast<int>((holdMs_ / 1000.0f) * sampleRate_);
}

void NoiseGate::reset() {
    envelope_ = 0.0f;
    holdCounter_ = 0;
    currentGain_ = 1.0f;
    targetGain_ = 1.0f;
    isOpen_ = true;
    gainReductionDb_ = 0.0f;
    lookaheadIndex_ = 0;
    for (int i = 0; i < kLookaheadSamples; ++i) {
        lookaheadBuffer_[i] = 0.0f;
    }
}

void NoiseGate::process(AudioBuffer& buffer) {
    if (bypass_) return;
    
    float* data = buffer.data;
    uint32_t numFrames = buffer.numFrames;
    
    float thresholdLinear = DSPUtils::dbToGain(thresholdDb_);
    float minGain = DSPUtils::dbToGain(rangeDb_);
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i];
        float inputAbs = std::abs(input);
        
        // Envelope follower (peak detection)
        if (inputAbs > envelope_) {
            envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * inputAbs;
        } else {
            envelope_ = releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * inputAbs;
        }
        
        // Determine gate state
        if (mode_ == Mode::Lookahead) {
            // Store in lookahead buffer
            lookaheadBuffer_[lookaheadIndex_] = input;
            lookaheadIndex_ = (lookaheadIndex_ + 1) % kLookaheadSamples;
            
            // Use envelope to determine gate state for future samples
            if (envelope_ > thresholdLinear) {
                targetGain_ = 1.0f;
                holdCounter_ = holdSamples_;
                isOpen_ = true;
            } else if (holdCounter_ > 0) {
                holdCounter_--;
            } else {
                targetGain_ = minGain;
                isOpen_ = false;
            }
            
            // Read from lookahead buffer
            float delayed = lookaheadBuffer_[lookaheadIndex_];
            
            // Smooth gain transitions
            float gainCoeff = targetGain_ > currentGain_ ? (1.0f - attackCoeff_) : (1.0f - releaseCoeff_);
            currentGain_ += (targetGain_ - currentGain_) * gainCoeff;
            
            data[i] = delayed * currentGain_;
        } else if (mode_ == Mode::Soft) {
            // Soft gate with smooth transitions
            if (envelope_ > thresholdLinear) {
                targetGain_ = 1.0f;
                holdCounter_ = holdSamples_;
                isOpen_ = true;
            } else if (holdCounter_ > 0) {
                holdCounter_--;
            } else {
                // Calculate soft knee based on distance from threshold
                float ratio = envelope_ / thresholdLinear;
                targetGain_ = minGain + (1.0f - minGain) * ratio;
                isOpen_ = targetGain_ > 0.5f;
            }
            
            // Smooth gain transitions
            float gainCoeff = targetGain_ > currentGain_ ? (1.0f - attackCoeff_) : (1.0f - releaseCoeff_);
            currentGain_ += (targetGain_ - currentGain_) * gainCoeff;
            
            data[i] = input * currentGain_;
        } else {
            // Hard gate
            if (envelope_ > thresholdLinear) {
                targetGain_ = 1.0f;
                holdCounter_ = holdSamples_;
                isOpen_ = true;
            } else if (holdCounter_ > 0) {
                holdCounter_--;
            } else {
                targetGain_ = minGain;
                isOpen_ = false;
            }
            
            // Smooth gain transitions (even hard gate needs some smoothing)
            float gainCoeff = targetGain_ > currentGain_ ? (1.0f - attackCoeff_) : (1.0f - releaseCoeff_);
            currentGain_ += (targetGain_ - currentGain_) * gainCoeff;
            
            data[i] = input * currentGain_;
        }
    }
    
    // Update gain reduction meter
    float currentDb = DSPUtils::gainToDb(currentGain_);
    gainReductionDb_ = -currentDb;
}

void NoiseGate::setThreshold(float thresholdDb) {
    thresholdDb_ = std::clamp(thresholdDb, -80.0f, 0.0f);
}

void NoiseGate::setAttack(float attackMs) {
    attackMs_ = std::clamp(attackMs, 0.1f, 50.0f);
    updateCoefficients();
}

void NoiseGate::setHold(float holdMs) {
    holdMs_ = std::clamp(holdMs, 0.0f, 500.0f);
    updateCoefficients();
}

void NoiseGate::setRelease(float releaseMs) {
    releaseMs_ = std::clamp(releaseMs, 10.0f, 1000.0f);
    updateCoefficients();
}

void NoiseGate::setRange(float rangeDb) {
    rangeDb_ = std::clamp(rangeDb, -80.0f, 0.0f);
}

void NoiseGate::setMode(Mode mode) {
    mode_ = mode;
}

void NoiseGate::setBypass(bool bypass) {
    bypass_ = bypass;
}

} // namespace openamp
