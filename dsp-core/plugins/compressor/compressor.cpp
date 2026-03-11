#include "compressor.h"
#include <cmath>
#include <algorithm>

namespace openamp {

Compressor::Compressor() {
    updateCoefficients();
}

void Compressor::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    updateCoefficients();
    reset();
}

void Compressor::updateCoefficients() {
    // Convert ms to coefficients for envelope follower
    float attackSamples = (attackMs_ / 1000.0f) * static_cast<float>(sampleRate_);
    float releaseSamples = (releaseMs_ / 1000.0f) * static_cast<float>(sampleRate_);
    
    attackCoeff_ = std::exp(-1.0f / attackSamples);
    releaseCoeff_ = std::exp(-1.0f / releaseSamples);
}

void Compressor::reset() {
    envelope_ = 0.0f;
    rmsIndex_ = 0;
    rmsSum_ = 0.0f;
    for (int i = 0; i < kRmsWindowSize; ++i) {
        rmsBuffer_[i] = 0.0f;
    }
    gainReductionDb_ = 0.0f;
    inputLevelDb_ = -60.0f;
    outputLevelDb_ = -60.0f;
    meterSmoothing_ = 0.0f;
    meterSmoothingOut_ = 0.0f;
}

float Compressor::detectLevel(float* data, uint32_t numFrames) {
    if (detector_ == Detector::Peak) {
        // Peak detection
        float peak = 0.0f;
        for (uint32_t i = 0; i < numFrames; ++i) {
            float absVal = std::abs(data[i]);
            if (absVal > peak) peak = absVal;
        }
        return peak;
    } else if (detector_ == Detector::RMS) {
        // RMS detection over window
        for (uint32_t i = 0; i < numFrames; ++i) {
            rmsSum_ -= rmsBuffer_[rmsIndex_];
            float squared = data[i] * data[i];
            rmsBuffer_[rmsIndex_] = squared;
            rmsSum_ += squared;
            rmsIndex_ = (rmsIndex_ + 1) % kRmsWindowSize;
        }
        return std::sqrt(rmsSum_ / kRmsWindowSize);
    }
    return 0.0f;
}

float Compressor::computeGainReduction(float inputDb) {
    if (inputDb <= thresholdDb_ - kneeDb_ / 2.0f) {
        // Below threshold - no compression
        return 0.0f;
    }
    
    if (mode_ == Mode::Limiter) {
        // Hard limiter - brick wall at threshold
        if (inputDb > thresholdDb_) {
            return thresholdDb_ - inputDb;
        }
        return 0.0f;
    }
    
    if (mode_ == Mode::HardKnee) {
        // Hard knee compression
        if (inputDb > thresholdDb_) {
            return (thresholdDb_ - inputDb) * (1.0f - 1.0f / ratio_);
        }
        return 0.0f;
    }
    
    // Soft knee compression
    if (inputDb < thresholdDb_ + kneeDb_ / 2.0f) {
        // In the knee region - smooth transition
        float kneeStart = thresholdDb_ - kneeDb_ / 2.0f;
        float kneePosition = (inputDb - kneeStart) / kneeDb_;
        float kneeGain = kneePosition * kneePosition / (2.0f * ratio_);
        return -kneeGain * (ratio_ - 1.0f);
    } else {
        // Above knee - full compression
        return (thresholdDb_ - inputDb) * (1.0f - 1.0f / ratio_);
    }
}

void Compressor::process(AudioBuffer& buffer) {
    if (bypass_) return;
    
    float* data = buffer.data;
    uint32_t numFrames = buffer.numFrames;
    
    float makeupGain = DSPUtils::dbToGain(makeupGainDb_);
    
    // Track input level for metering
    float inputPeak = 0.0f;
    float outputPeak = 0.0f;
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i];
        float inputAbs = std::abs(input);
        
        // Track input level
        if (inputAbs > inputPeak) inputPeak = inputAbs;
        
        // Compute input level in dB
        float inputLevel = inputAbs > 1e-10f ? inputAbs : 1e-10f;
        
        // Feedback detector uses envelope instead of input
        float detected;
        if (detector_ == Detector::Feedback) {
            detected = envelope_;
        } else {
            detected = inputLevel;
        }
        
        // Envelope follower
        if (detected > envelope_) {
            envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * detected;
        } else {
            envelope_ = releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * detected;
        }
        
        // Convert envelope to dB
        float envelopeDb = DSPUtils::gainToDb(envelope_);
        
        // Compute gain reduction
        float grDb = computeGainReduction(envelopeDb);
        gainReductionDb_ = grDb;
        
        // Apply gain reduction and makeup gain
        float gainReduction = DSPUtils::dbToGain(grDb);
        float compressed = input * gainReduction * makeupGain;
        
        // Mix dry/wet
        float output = input * (1.0f - mix_) + compressed * mix_;
        
        // Track output level
        float outputAbs = std::abs(output);
        if (outputAbs > outputPeak) outputPeak = outputAbs;
        
        data[i] = output;
    }
    
    // Update level meters with smoothing
    float meterSmoothingCoeff = 0.9f;
    float inputDb = DSPUtils::gainToDb(inputPeak);
    float outputDb = DSPUtils::gainToDb(outputPeak);
    
    inputLevelDb_ = meterSmoothing_ * meterSmoothingCoeff + inputDb * (1.0f - meterSmoothingCoeff);
    outputLevelDb_ = meterSmoothingOut_ * meterSmoothingCoeff + outputDb * (1.0f - meterSmoothingCoeff);
    meterSmoothing_ = inputLevelDb_;
    meterSmoothingOut_ = outputLevelDb_;
}

void Compressor::setThreshold(float thresholdDb) {
    thresholdDb_ = std::clamp(thresholdDb, -60.0f, 0.0f);
}

void Compressor::setRatio(float ratio) {
    ratio_ = std::clamp(ratio, 1.0f, 20.0f);
}

void Compressor::setAttack(float attackMs) {
    attackMs_ = std::clamp(attackMs, 0.1f, 100.0f);
    updateCoefficients();
}

void Compressor::setRelease(float releaseMs) {
    releaseMs_ = std::clamp(releaseMs, 10.0f, 1000.0f);
    updateCoefficients();
}

void Compressor::setKnee(float kneeDb) {
    kneeDb_ = std::clamp(kneeDb, 0.0f, 24.0f);
}

void Compressor::setMakeupGain(float gainDb) {
    makeupGainDb_ = std::clamp(gainDb, 0.0f, 24.0f);
}

void Compressor::setMix(float mix) {
    mix_ = std::clamp(mix, 0.0f, 1.0f);
}

void Compressor::setMode(Mode mode) {
    mode_ = mode;
}

void Compressor::setDetector(Detector detector) {
    detector_ = detector;
}

void Compressor::setBypass(bool bypass) {
    bypass_ = bypass;
}

} // namespace openamp
