#include "delay.h"
#include <algorithm>
#include <cmath>

namespace openamp {

Delay::Delay() = default;

void Delay::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    (void)maxBlockSize;
    
    // Max 2 seconds of delay
    maxDelaySamples_ = static_cast<uint32_t>(sampleRate * 2.0);
    
    // Initialize delay lines
    ensureChannels(2);
    
    updateDelaySamples();
}

void Delay::reset() {
    for (auto& line : delayLines_) {
        std::fill(line.begin(), line.end(), 0.0f);
    }
    for (auto& idx : writeIndices_) {
        idx = 0;
    }
    for (auto& filter : highPassFilters_) {
        filter.reset();
    }
    for (auto& filter : lowPassFilters_) {
        filter.reset();
    }
    lfoPhase_ = 0.0f;
}

void Delay::ensureChannels(uint32_t numChannels) {
    if (delayLines_.size() < numChannels) {
        delayLines_.resize(numChannels);
        writeIndices_.resize(numChannels, 0);
        highPassFilters_.resize(numChannels);
        lowPassFilters_.resize(numChannels);
        
        for (auto& line : delayLines_) {
            line.resize(maxDelaySamples_, 0.0f);
        }
        
        for (auto& filter : highPassFilters_) {
            filter.setCutoff(highPassHz_, static_cast<float>(sampleRate_));
        }
        for (auto& filter : lowPassFilters_) {
            filter.setCutoff(lowPassHz_, static_cast<float>(sampleRate_));
        }
    }
}

void Delay::updateDelaySamples() {
    delaySamples_ = static_cast<float>(timeMs_ / 1000.0 * sampleRate_);
    delaySamples_ = std::clamp(delaySamples_, 1.0f, static_cast<float>(maxDelaySamples_ - 1));
}

float Delay::readInterpolated(const std::vector<float>& buffer, uint32_t writeIndex, float delaySamples) {
    int bufferSize = static_cast<int>(buffer.size());
    
    // Calculate read position
    float readPos = static_cast<float>(writeIndex) - delaySamples;
    while (readPos < 0.0f) readPos += bufferSize;
    
    int readIndex = static_cast<int>(readPos);
    float frac = readPos - readIndex;
    
    // Linear interpolation
    int idx0 = readIndex % bufferSize;
    int idx1 = (readIndex + 1) % bufferSize;
    
    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

void Delay::process(AudioBuffer& buffer) {
    if (buffer.numChannels == 0 || buffer.numFrames == 0) return;
    
    ensureChannels(buffer.numChannels);
    
    // Update LFO for tape mode
    float lfoValue = 0.0f;
    if (mode_ == Mode::Tape) {
        lfoPhase_ += lfoRate_ / static_cast<float>(sampleRate_);
        if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
        lfoValue = std::sin(lfoPhase_ * 6.28318530718f) * lfoDepth_ * delaySamples_;
    }
    
    for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
        float* channelData = buffer.data + ch * buffer.numFrames;
        auto& delayLine = delayLines_[ch];
        uint32_t& writeIdx = writeIndices_[ch];
        
        for (uint32_t i = 0; i < buffer.numFrames; ++i) {
            float input = channelData[i];
            
            // Calculate modulated delay time for tape mode
            float currentDelay = delaySamples_;
            if (mode_ == Mode::Tape) {
                currentDelay += lfoValue;
                currentDelay = std::clamp(currentDelay, 1.0f, static_cast<float>(maxDelaySamples_ - 1));
            }
            
            // Read from delay line
            float delayed = readInterpolated(delayLine, writeIdx, currentDelay);
            
            // Apply feedback filters
            float feedbackSignal = delayed * feedback_;
            feedbackSignal = highPassFilters_[ch].process(feedbackSignal);
            feedbackSignal = lowPassFilters_[ch].process(feedbackSignal);
            
            // Write to delay line
            delayLine[writeIdx] = input + feedbackSignal;
            writeIdx = (writeIdx + 1) % maxDelaySamples_;
            
            // Mix dry/wet
            float output = input * (1.0f - mix_) + delayed * mix_;
            
            // Slapback mode: no feedback, fixed time
            if (mode_ == Mode::Slapback) {
                output = input * 0.7f + delayed * 0.3f;
            }
            
            channelData[i] = output;
        }
    }
    
    // Ping-pong mode: cross feedback between channels
    if (mode_ == Mode::PingPong && buffer.numChannels >= 2) {
        // Swap delay line contents for next iteration
        std::swap(delayLines_[0], delayLines_[1]);
    }
}

void Delay::setTimeMs(float ms) {
    timeMs_ = std::clamp(ms, 10.0f, 2000.0f);
    updateDelaySamples();
}

void Delay::setFeedback(float amount) {
    feedback_ = std::clamp(amount, 0.0f, 0.95f);
}

void Delay::setMix(float amount) {
    mix_ = std::clamp(amount, 0.0f, 1.0f);
}

void Delay::setMode(Mode mode) {
    mode_ = mode;
}

void Delay::setHighPass(float hz) {
    highPassHz_ = std::clamp(hz, 20.0f, 500.0f);
    for (auto& filter : highPassFilters_) {
        filter.setCutoff(highPassHz_, static_cast<float>(sampleRate_));
    }
}

void Delay::setLowPass(float hz) {
    lowPassHz_ = std::clamp(hz, 1000.0f, 16000.0f);
    for (auto& filter : lowPassFilters_) {
        filter.setCutoff(lowPassHz_, static_cast<float>(sampleRate_));
    }
}

} // namespace openamp
