#include "wah.h"
#include <cmath>
#include <algorithm>

namespace openamp {

Wah::Wah() {
    updateCoefficients();
}

void Wah::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    updateCoefficients();
    reset();
}

void Wah::updateCoefficients() {
    lfoIncrement_ = rate_ / static_cast<float>(sampleRate_);
    
    float attackMs = 5.0f;
    float releaseMs = 100.0f;
    attackCoeff_ = std::exp(-1.0f / ((attackMs / 1000.0f) * sampleRate_));
    releaseCoeff_ = std::exp(-1.0f / ((releaseMs / 1000.0f) * sampleRate_));
}

void Wah::reset() {
    x1_ = x2_ = y1_ = y2_ = 0.0f;
    lfoPhase_ = 0.0f;
    envelope_ = 0.0f;
}

void Wah::updateFilter(float centerFreq) {
    constexpr float kPi = 3.14159265358979323846f;
    
    float omega = 2.0f * kPi * centerFreq / static_cast<float>(sampleRate_);
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * q_);
    
    // Bandpass filter coefficients
    float a0 = 1.0f + alpha;
    b0_ = alpha / a0;
    b1_ = 0.0f;
    b2_ = -alpha / a0;
    a1_ = -2.0f * cosOmega / a0;
    a2_ = (1.0f - alpha) / a0;
}

void Wah::process(AudioBuffer& buffer) {
    if (bypass_) return;
    
    float* data = buffer.data;
    uint32_t numFrames = buffer.numFrames;
    
    // Frequency range for wah (400 Hz to 2500 Hz typical)
    float minFreq = 400.0f;
    float maxFreq = 2500.0f;
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i];
        float wahPosition = position_;
        
        // Determine wah position based on mode
        if (mode_ == Mode::Auto) {
            // LFO-driven auto-wah
            float lfo = std::sin(2.0f * 3.14159265358979323846f * lfoPhase_);
            lfo = (lfo + 1.0f) * 0.5f;  // 0 to 1
            wahPosition = 0.5f + (lfo - 0.5f) * depth_;
            lfoPhase_ += lfoIncrement_;
            if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
        } else if (mode_ == Mode::Touch) {
            // Envelope follower touch wah
            float inputAbs = std::abs(input);
            if (inputAbs > envelope_) {
                envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * inputAbs;
            } else {
                envelope_ = releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * inputAbs;
            }
            
            // Map envelope to wah position
            float envNorm = envelope_ * sensitivity_ * 10.0f;  // Scale up for sensitivity
            wahPosition = std::clamp(envNorm, 0.0f, 1.0f);
        }
        
        // Calculate center frequency from position
        float centerFreq = minFreq + (maxFreq - minFreq) * wahPosition;
        
        // Update filter coefficients
        updateFilter(centerFreq);
        
        // Apply bandpass filter
        float output = b0_ * input + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        x2_ = x1_;
        x1_ = input;
        y2_ = y1_;
        y1_ = output;
        
        // Mix dry/wet
        data[i] = input * (1.0f - mix_) + output * mix_;
    }
}

void Wah::setPosition(float position) {
    position_ = std::clamp(position, 0.0f, 1.0f);
}

void Wah::setDepth(float depth) {
    depth_ = std::clamp(depth, 0.0f, 1.0f);
}

void Wah::setRate(float hz) {
    rate_ = std::clamp(hz, 0.1f, 10.0f);
    updateCoefficients();
}

void Wah::setSensitivity(float sensitivity) {
    sensitivity_ = std::clamp(sensitivity, 0.0f, 1.0f);
}

void Wah::setQ(float q) {
    q_ = std::clamp(q, 0.5f, 20.0f);
}

void Wah::setMix(float mix) {
    mix_ = std::clamp(mix, 0.0f, 1.0f);
}

void Wah::setMode(Mode mode) {
    mode_ = mode;
}

void Wah::setBypass(bool bypass) {
    bypass_ = bypass;
}

} // namespace openamp
