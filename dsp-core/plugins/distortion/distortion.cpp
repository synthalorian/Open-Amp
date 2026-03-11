#include "distortion.h"
#include "openamp/simd_utils.h"
#include <algorithm>
#include <cmath>

namespace openamp {

Distortion::Distortion() = default;

void Distortion::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    (void)maxBlockSize;
    
    // Set up tone filter
    float cutoff = 1000.0f + tone_ * 6000.0f;
    toneFilter_.setCutoff(cutoff, static_cast<float>(sampleRate));
}

void Distortion::reset() {
    toneFilter_.reset();
}

void Distortion::process(AudioBuffer& buffer) {
    if (buffer.numChannels == 0 || buffer.numFrames == 0) return;
    
    for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
        float* channelData = buffer.data + ch * buffer.numFrames;
        
        // Use SIMD for overdrive type (most common)
        if (type_ == Type::Overdrive && buffer.numFrames >= 4) {
            processOverdriveSIMD(channelData, buffer.numFrames);
        } else {
            // Scalar fallback for other types
            for (uint32_t i = 0; i < buffer.numFrames; ++i) {
                float sample = processSample(channelData[i]);
                sample = toneFilter_.process(sample);
                channelData[i] = sample * level_;
            }
        }
    }
}

void Distortion::processOverdriveSIMD(float* data, uint32_t numFrames) {
    using namespace simd;
    
    v4f driveVec = set1(1.0f + drive_ * 15.0f);
    v4f levelVec = set1(level_);
    v4f twentyseven = set1(27.0f);
    v4f twentySix = set1(26.0f);
    v4f onePointOne = set1(1.1f);
    v4f zeroEight = set1(0.8f);
    
    uint32_t i = 0;
    
    // Process 4 samples at a time
    for (; i + 4 <= numFrames; i += 4) {
        v4f samples = load(data + i);
        
        // Apply drive
        samples = mul(samples, driveVec);
        samples = mul(samples, zeroEight);
        
        // Tube Screamer-style overdrive: x * (27 + |x|) / (27 + 26*|x|)
        v4f absX = abs(samples);
        v4f num = mul(samples, add(twentyseven, absX));
        v4f denom = add(twentyseven, mul(twentySix, absX));
        samples = mul(div(num, denom), onePointOne);
        
        // Apply tone filter (scalar for now - could be vectorized)
        float temp[4];
        store(temp, samples);
        for (int j = 0; j < 4; ++j) {
            temp[j] = toneFilter_.process(temp[j]);
        }
        samples = load(temp);
        
        // Apply level
        store(data + i, mul(samples, levelVec));
    }
    
    // Process remaining samples
    for (; i < numFrames; ++i) {
        float sample = processSample(data[i]);
        sample = toneFilter_.process(sample);
        data[i] = sample * level_;
    }
}

float Distortion::processSample(float input) {
    float driveAmount = 1.0f + drive_ * 15.0f;
    float sample = input * driveAmount;
    
    switch (type_) {
        case Type::Overdrive: {
            // Smooth asymmetrical overdrive (Tube Screamer style)
            float x = sample * 0.8f;
            float y = x * (27.0f + std::abs(x)) / (27.0f + 26.0f * std::abs(x));
            sample = y * 1.1f;
            break;
        }
        
        case Type::Fuzz: {
            // Octave fuzz with transistor-style clipping
            float x = std::tanh(sample * 0.5f);
            float rectified = std::abs(x);
            float octave = x * x * std::copysign(1.0f, x);
            sample = (x * 0.7f + octave * 0.3f) * 1.5f;
            sample = std::tanh(sample);
            break;
        }
        
        case Type::Tube: {
            // Asymmetrical tube-like saturation
            float x = sample;
            if (x > 0.0f) {
                // Positive half - softer saturation
                sample = 1.0f - std::exp(-x * 0.8f);
            } else {
                // Negative half - harder saturation
                sample = -(1.0f - std::exp(x * 1.2f));
            }
            sample *= 0.9f;
            break;
        }
        
        case Type::HardClip: {
            // Aggressive digital hard clipping
            float threshold = 0.7f - drive_ * 0.3f;
            threshold = std::max(0.2f, threshold);
            
            if (sample > threshold) {
                sample = threshold;
            } else if (sample < -threshold) {
                sample = -threshold;
            }
            
            // Add some character
            sample = std::tanh(sample * 1.2f);
            break;
        }
    }
    
    // Soft limit to prevent harsh clipping
    sample = std::tanh(sample * 0.9f);
    
    return sample;
}

void Distortion::setDrive(float amount) {
    drive_ = std::clamp(amount, 0.0f, 1.0f);
}

void Distortion::setTone(float amount) {
    tone_ = std::clamp(amount, 0.0f, 1.0f);
    
    // Map tone to cutoff frequency (800Hz - 8kHz)
    float cutoff = 800.0f + tone_ * 7200.0f;
    toneFilter_.setCutoff(cutoff, static_cast<float>(sampleRate_));
}

void Distortion::setLevel(float amount) {
    level_ = std::clamp(amount, 0.0f, 1.0f);
}

void Distortion::setType(Type type) {
    type_ = type;
}

} // namespace openamp
