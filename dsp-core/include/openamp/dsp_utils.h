#pragma once

#include <cmath>

namespace openamp {

class DSPUtils {
public:
    static inline float dbToGain(float db) {
        return std::pow(10.0f, db / 20.0f);
    }
    
    static inline float gainToDb(float gain) {
        return 20.0f * std::log10(gain);
    }
    
    static inline float hardClip(float sample, float threshold) {
        if (sample > threshold) return threshold;
        if (sample < -threshold) return -threshold;
        return sample;
    }
    
    static inline float softClip(float sample, float drive) {
        float x = sample * drive;
        return x / (1.0f + std::abs(x));
    }
    
    static inline float tanhClip(float sample, float drive) {
        return std::tanh(sample * drive);
    }
};

class OnePoleFilter {
public:
    void setCutoff(float cutoffHz, float sampleRate) {
        constexpr float kPi = 3.14159265358979323846f;
        float omega = 2.0f * kPi * cutoffHz / sampleRate;
        a1 = (1.0f - std::sin(omega)) / std::cos(omega);
        b0 = (1.0f - a1) / 2.0f;
        b1 = b0;
    }
    
    inline float process(float input) {
        float output = b0 * input + b1 * z1;
        z1 = input - a1 * output;
        return output;
    }
    
    void reset() { z1 = 0.0f; }
    
private:
    float a1 = 0.0f, b0 = 0.5f, b1 = 0.5f;
    float z1 = 0.0f;
};

} // namespace openamp
