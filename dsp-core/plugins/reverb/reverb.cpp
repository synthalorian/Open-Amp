#include "reverb.h"
#include <algorithm>
#include <cmath>

namespace openamp {

// ============== Helper structures ==============

void Reverb::Comb::resize(uint32_t size) {
    buffer.resize(size, 0.0f);
    index = 0;
}

float Reverb::Comb::process(float input) {
    float output = buffer[index];
    filterStore = output * (1.0f - damp) + filterStore * damp;
    buffer[index] = input + filterStore * feedback;
    index = (index + 1) % static_cast<uint32_t>(buffer.size());
    return output;
}

void Reverb::Comb::reset() {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    index = 0;
    filterStore = 0.0f;
}

void Reverb::Allpass::resize(uint32_t size) {
    buffer.resize(size, 0.0f);
    index = 0;
}

float Reverb::Allpass::process(float input) {
    float output = buffer[index];
    float bufOut = output * -feedback;
    buffer[index] = input + bufOut;
    index = (index + 1) % static_cast<uint32_t>(buffer.size());
    return output + bufOut;
}

void Reverb::Allpass::reset() {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    index = 0;
}

void Reverb::DelayLine::resize(uint32_t size) {
    buffer.resize(size, 0.0f);
    index = 0;
}

void Reverb::DelayLine::write(float sample) {
    buffer[index] = sample;
    index = (index + 1) % static_cast<uint32_t>(buffer.size());
}

float Reverb::DelayLine::read(uint32_t delay) const {
    uint32_t readIdx = (index + buffer.size() - delay) % buffer.size();
    return buffer[readIdx];
}

void Reverb::DelayLine::reset() {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    index = 0;
}

// ============== Reverb ==============

Reverb::Reverb() {
    // Early reflection tap times (in samples at 44.1kHz, scaled later)
    earlyTapTimes_ = {19, 43, 67, 89, 127, 163, 211, 277};
    earlyTapGains_ = {0.8f, 0.6f, 0.5f, 0.4f, 0.35f, 0.3f, 0.25f, 0.2f};
}

void Reverb::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    (void)maxBlockSize;
    
    ensureInitialized();
    updateParameters();
}

void Reverb::ensureInitialized() {
    float srFactor = static_cast<float>(sampleRate_ / 44100.0);
    
    // Comb filter sizes (stereo offset for width)
    uint32_t combSizesL[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    uint32_t combSizesR[] = {1139, 1211, 1300, 1379, 1445, 1514, 1580, 1640};
    
    combsL_.resize(8);
    combsR_.resize(8);
    for (int i = 0; i < 8; ++i) {
        combsL_[i].resize(static_cast<uint32_t>(combSizesL[i] * srFactor));
        combsR_[i].resize(static_cast<uint32_t>(combSizesR[i] * srFactor));
    }
    
    // Allpass filter sizes
    uint32_t allpassSizesL[] = {556, 441, 341, 225};
    uint32_t allpassSizesR[] = {579, 464, 364, 248};
    
    allpassesL_.resize(4);
    allpassesR_.resize(4);
    for (int i = 0; i < 4; ++i) {
        allpassesL_[i].resize(static_cast<uint32_t>(allpassSizesL[i] * srFactor));
        allpassesR_[i].resize(static_cast<uint32_t>(allpassSizesR[i] * srFactor));
        allpassesL_[i].feedback = 0.5f;
        allpassesR_[i].feedback = 0.5f;
    }
    
    // Pre-delay (max 200ms)
    preDelay_.resize(static_cast<uint32_t>(sampleRate_ * 0.2));
    
    // Early reflections delay
    earlyRef_.resize(static_cast<uint32_t>(500 * srFactor));
}

void Reverb::updateParameters() {
    float feedback = 0.7f + roomSize_ * 0.28f;
    float damp = damping_ * 0.4f;
    
    for (auto& comb : combsL_) {
        comb.feedback = feedback;
        comb.damp = damp;
    }
    for (auto& comb : combsR_) {
        comb.feedback = feedback;
        comb.damp = damp;
    }
}

void Reverb::reset() {
    for (auto& comb : combsL_) comb.reset();
    for (auto& comb : combsR_) comb.reset();
    for (auto& ap : allpassesL_) ap.reset();
    for (auto& ap : allpassesR_) ap.reset();
    preDelay_.reset();
    earlyRef_.reset();
}

void Reverb::process(AudioBuffer& buffer) {
    if (buffer.numChannels == 0 || buffer.numFrames == 0) return;
    
    // Ensure we're initialized
    if (combsL_.empty()) {
        ensureInitialized();
    }
    
    uint32_t preDelaySamples = static_cast<uint32_t>(preDelayMs_ / 1000.0f * sampleRate_);
    
    for (uint32_t i = 0; i < buffer.numFrames; ++i) {
        float input = 0.0f;
        for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
            input += buffer.data[ch * buffer.numFrames + i];
        }
        input /= static_cast<float>(buffer.numChannels);
        
        // Pre-delay
        preDelay_.write(input);
        float delayed = preDelay_.read(preDelaySamples);
        
        // Early reflections (simplified)
        earlyRef_.write(delayed);
        float early = 0.0f;
        for (size_t t = 0; t < earlyTapTimes_.size(); ++t) {
            uint32_t tapTime = static_cast<uint32_t>(earlyTapTimes_[t] * sampleRate_ / 44100.0);
            early += earlyRef_.read(tapTime) * earlyTapGains_[t];
        }
        early *= 0.3f;
        
        // Late reverb
        float lateL = 0.0f;
        float lateR = 0.0f;
        
        for (auto& comb : combsL_) {
            lateL += comb.process(delayed);
        }
        for (auto& comb : combsR_) {
            lateR += comb.process(delayed);
        }
        
        for (auto& ap : allpassesL_) {
            lateL = ap.process(lateL);
        }
        for (auto& ap : allpassesR_) {
            lateR = ap.process(lateR);
        }
        
        // Scale late reverb
        lateL *= 0.015f;
        lateR *= 0.015f;
        
        // Apply width
        float mid = (lateL + lateR) * 0.5f;
        float side = (lateL - lateR) * 0.5f * width_;
        lateL = mid + side;
        lateR = mid - side;
        
        // Combine early + late
        float outL = early + lateL;
        float outR = early + lateR;
        
        // Mix with dry signal
        if (buffer.numChannels >= 2) {
            buffer.data[0 * buffer.numFrames + i] = 
                buffer.data[0 * buffer.numFrames + i] * (1.0f - mix_) + outL * mix_;
            buffer.data[1 * buffer.numFrames + i] = 
                buffer.data[1 * buffer.numFrames + i] * (1.0f - mix_) + outR * mix_;
        } else {
            float monoOut = (outL + outR) * 0.5f;
            buffer.data[i] = buffer.data[i] * (1.0f - mix_) + monoOut * mix_;
        }
    }
}

void Reverb::processSpring(float* data, uint32_t numFrames) {
    // Simplified spring reverb simulation
    // Uses shorter, more "bouncy" delay times
    for (uint32_t i = 0; i < numFrames; ++i) {
        // Add spring-like characteristics
        float input = data[i];
        float output = input * 0.8f;
        
        // Add some "boing"
        static float lastSample = 0.0f;
        static float accumulator = 0.0f;
        
        accumulator = accumulator * 0.95f + (input - lastSample) * 0.3f;
        output += accumulator * 0.2f;
        
        lastSample = input;
        data[i] = data[i] * (1.0f - mix_) + output * mix_;
    }
}

void Reverb::setRoomSize(float amount) {
    roomSize_ = std::clamp(amount, 0.0f, 1.0f);
    updateParameters();
}

void Reverb::setDamping(float amount) {
    damping_ = std::clamp(amount, 0.0f, 1.0f);
    updateParameters();
}

void Reverb::setMix(float amount) {
    mix_ = std::clamp(amount, 0.0f, 1.0f);
}

void Reverb::setPreDelay(float ms) {
    preDelayMs_ = std::clamp(ms, 0.0f, 200.0f);
}

void Reverb::setWidth(float amount) {
    width_ = std::clamp(amount, 0.0f, 2.0f);
}

void Reverb::setType(Type type) {
    type_ = type;
}

} // namespace openamp
