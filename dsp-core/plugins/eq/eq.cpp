#include "eq.h"
#include <cmath>

namespace openamp {

// BiquadFilter implementation
void BiquadFilter::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void BiquadFilter::updateCoefficients() {
    constexpr float kPi = 3.14159265358979323846f;
    
    float omega = 2.0f * kPi * freq_ / static_cast<float>(sampleRate_);
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * q_);
    float A = std::pow(10.0f, gainDb_ / 40.0f);
    
    switch (type_) {
        case Type::LowPass: {
            float b0 = (1.0f - cosOmega) / 2.0f;
            float b1 = 1.0f - cosOmega;
            float b2 = (1.0f - cosOmega) / 2.0f;
            float a0 = 1.0f + alpha;
            b0_ = b0 / a0;
            b1_ = b1 / a0;
            b2_ = b2 / a0;
            a1_ = -2.0f * cosOmega / a0;
            a2_ = (1.0f - alpha) / a0;
            break;
        }
        case Type::HighPass: {
            float b0 = (1.0f + cosOmega) / 2.0f;
            float b1 = -(1.0f + cosOmega);
            float b2 = (1.0f + cosOmega) / 2.0f;
            float a0 = 1.0f + alpha;
            b0_ = b0 / a0;
            b1_ = b1 / a0;
            b2_ = b2 / a0;
            a1_ = -2.0f * cosOmega / a0;
            a2_ = (1.0f - alpha) / a0;
            break;
        }
        case Type::BandPass: {
            b0_ = alpha;
            b1_ = 0.0f;
            b2_ = -alpha;
            float a0 = 1.0f + alpha;
            a1_ = -2.0f * cosOmega / a0;
            a2_ = (1.0f - alpha) / a0;
            b0_ /= a0;
            b2_ /= a0;
            break;
        }
        case Type::Notch: {
            b0_ = 1.0f;
            b1_ = -2.0f * cosOmega;
            b2_ = 1.0f;
            float a0 = 1.0f + alpha;
            b0_ /= a0;
            b1_ /= a0;
            b2_ /= a0;
            a1_ = -2.0f * cosOmega / a0;
            a2_ = (1.0f - alpha) / a0;
            break;
        }
        case Type::Peaking: {
            float a0 = 1.0f + alpha / A;
            b0_ = (1.0f + alpha * A) / a0;
            b1_ = (-2.0f * cosOmega) / a0;
            b2_ = (1.0f - alpha * A) / a0;
            a1_ = (-2.0f * cosOmega) / a0;
            a2_ = (1.0f - alpha / A) / a0;
            break;
        }
        case Type::LowShelf: {
            float beta = std::sqrt(A + A);
            float a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + beta * sinOmega;
            b0_ = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + beta * sinOmega) / a0;
            b1_ = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega) / a0;
            b2_ = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - beta * sinOmega) / a0;
            a1_ = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega) / a0;
            a2_ = ((A + 1.0f) + (A - 1.0f) * cosOmega - beta * sinOmega) / a0;
            break;
        }
        case Type::HighShelf: {
            float beta = std::sqrt(A + A);
            float a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + beta * sinOmega;
            b0_ = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + beta * sinOmega) / a0;
            b1_ = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega) / a0;
            b2_ = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - beta * sinOmega) / a0;
            a1_ = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega) / a0;
            a2_ = ((A + 1.0f) - (A - 1.0f) * cosOmega - beta * sinOmega) / a0;
            break;
        }
    }
}

float BiquadFilter::process(float input) {
    float output = b0_ * input + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
    x2_ = x1_;
    x1_ = input;
    y2_ = y1_;
    y1_ = output;
    return output;
}

void BiquadFilter::reset() {
    x1_ = x2_ = y1_ = y2_ = 0.0f;
}

// EQ implementation
EQ::EQ() {
    // Initialize bands with default frequencies (10-band graphic EQ style)
    float freqs[kNumBands] = {31.0f, 63.0f, 125.0f, 250.0f, 500.0f, 
                               1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
    
    for (size_t i = 0; i < kNumBands; ++i) {
        bands_[i].frequency = freqs[i];
        bands_[i].filter.setFrequency(freqs[i]);
        bands_[i].filter.setType(BiquadFilter::Type::Peaking);
        bands_[i].filter.setQ(1.414f);  // ~2 octaves bandwidth for graphic EQ
    }
}

void EQ::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    for (auto& band : bands_) {
        band.filter.prepare(sampleRate);
    }
}

void EQ::process(AudioBuffer& buffer) {
    if (bypass_) return;
    
    float* data = buffer.data;
    uint32_t numFrames = buffer.numFrames;
    
    float outputGain = DSPUtils::dbToGain(outputGainDb_);
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = data[i];
        
        for (auto& band : bands_) {
            if (band.enabled) {
                sample = band.filter.process(sample);
            }
        }
        
        data[i] = sample * outputGain;
    }
}

void EQ::reset() {
    for (auto& band : bands_) {
        band.filter.reset();
    }
}

void EQ::setBandEnabled(size_t band, bool enabled) {
    if (band < kNumBands) {
        bands_[band].enabled = enabled;
    }
}

void EQ::setBandFrequency(size_t band, float freqHz) {
    if (band < kNumBands) {
        bands_[band].frequency = freqHz;
        bands_[band].filter.setFrequency(freqHz);
        bands_[band].filter.prepare(sampleRate_);
    }
}

void EQ::setBandGain(size_t band, float gainDb) {
    if (band < kNumBands) {
        bands_[band].gain = gainDb;
        bands_[band].filter.setGain(gainDb);
        bands_[band].filter.prepare(sampleRate_);
    }
}

void EQ::setBandQ(size_t band, float q) {
    if (band < kNumBands) {
        bands_[band].q = q;
        bands_[band].filter.setQ(q);
        bands_[band].filter.prepare(sampleRate_);
    }
}

void EQ::setBandType(size_t band, BiquadFilter::Type type) {
    if (band < kNumBands) {
        bands_[band].type = type;
        bands_[band].filter.setType(type);
        bands_[band].filter.prepare(sampleRate_);
    }
}

bool EQ::getBandEnabled(size_t band) const {
    return band < kNumBands ? bands_[band].enabled : false;
}

float EQ::getBandFrequency(size_t band) const {
    return band < kNumBands ? bands_[band].frequency : 0.0f;
}

float EQ::getBandGain(size_t band) const {
    return band < kNumBands ? bands_[band].gain : 0.0f;
}

float EQ::getBandQ(size_t band) const {
    return band < kNumBands ? bands_[band].q : 1.0f;
}

BiquadFilter::Type EQ::getBandType(size_t band) const {
    return band < kNumBands ? bands_[band].type : BiquadFilter::Type::Peaking;
}

void EQ::setLowCut(float freqHz, float /*slope*/) {
    // Use band 0 for low cut
    bands_[0].filter.setType(BiquadFilter::Type::HighPass);
    bands_[0].filter.setFrequency(freqHz);
    bands_[0].filter.setQ(0.707f);
    bands_[0].filter.prepare(sampleRate_);
    bands_[0].enabled = true;
}

void EQ::setHighCut(float freqHz, float /*slope*/) {
    // Use band 9 for high cut
    bands_[9].filter.setType(BiquadFilter::Type::LowPass);
    bands_[9].filter.setFrequency(freqHz);
    bands_[9].filter.setQ(0.707f);
    bands_[9].filter.prepare(sampleRate_);
    bands_[9].enabled = true;
}

void EQ::setOutputGain(float gainDb) {
    outputGainDb_ = gainDb;
}

void EQ::setBypass(bool bypass) {
    bypass_ = bypass;
}

} // namespace openamp
