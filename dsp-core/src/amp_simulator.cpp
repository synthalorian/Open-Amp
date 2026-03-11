#include "openamp/amp_simulator.h"
#include <algorithm>
#include <cmath>

namespace openamp {

AmpSimulator::AmpSimulator() = default;

void AmpSimulator::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    reset();
    
    presenceFilter_.setCutoff(5000.0f, static_cast<float>(sampleRate));
    cabHighPass_.setCutoff(80.0f, static_cast<float>(sampleRate));
    cabLowPass_.setCutoff(6000.0f, static_cast<float>(sampleRate));
    toneStack_.setParameters(bassDb_, midDb_, trebleDb_, static_cast<float>(sampleRate));
}

void AmpSimulator::reset() {
    toneStack_.reset();
    presenceFilter_.reset();
    cabHighPass_.reset();
    cabLowPass_.reset();
    cabHistory_.assign(cabIR_.size(), 0.0f);
    cabIndex_ = 0;
    preEmphasisState_ = 0.0f;
    deEmphasisState_ = 0.0f;
}

void AmpSimulator::process(AudioBuffer& buffer) {
    if (buffer.numChannels == 0 || buffer.numFrames == 0) return;
    
    for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
        float* channelData = buffer.data + ch * buffer.numFrames;
        
        processPreamp(channelData, buffer.numFrames);
        
        for (uint32_t i = 0; i < buffer.numFrames; ++i) {
            channelData[i] = toneStack_.process(channelData[i]);
            channelData[i] = presenceFilter_.process(channelData[i]);
        }
        
        processPowerAmp(channelData, buffer.numFrames);

        for (uint32_t i = 0; i < buffer.numFrames; ++i) {
            float sample = channelData[i];
            float hp = sample - cabHighPass_.process(sample);
            float cab = cabLowPass_.process(hp);

            if (!cabIR_.empty()) {
                cabHistory_[cabIndex_] = cab;
                float acc = 0.0f;
                size_t idx = cabIndex_;
                for (size_t k = 0; k < cabIR_.size(); ++k) {
                    acc += cabIR_[k] * cabHistory_[idx];
                    idx = (idx == 0) ? (cabIR_.size() - 1) : (idx - 1);
                }
                cabIndex_ = (cabIndex_ + 1) % cabIR_.size();
                channelData[i] = acc;
            } else {
                channelData[i] = cab;
            }
        }
    }
}

void AmpSimulator::setCabIR(const std::vector<float>& ir) {
    cabIR_ = ir;
    cabHistory_.assign(cabIR_.size(), 0.0f);
    cabIndex_ = 0;
}

void AmpSimulator::processPreamp(float* data, uint32_t numFrames) {
    const float preGain = DSPUtils::dbToGain(gain_ * 40.0f - 20.0f);
    const float driveAmount = 1.0f + drive_ * 9.0f;
    constexpr float kPi = 3.14159265358979323846f;
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = data[i] * preGain;
        
        sample = DSPUtils::tanhClip(sample, driveAmount);
        
        const float emphasisFreq = 2000.0f;
        const float emphasisCoeff = std::exp(-2.0f * kPi * emphasisFreq / sampleRate_);
        sample = sample - emphasisCoeff * preEmphasisState_;
        preEmphasisState_ = sample;
        
        data[i] = sample;
    }
}

void AmpSimulator::processPowerAmp(float* data, uint32_t numFrames) {
    const float masterGain = DSPUtils::dbToGain(master_ * 20.0f - 10.0f);
    constexpr float kPi = 3.14159265358979323846f;
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = data[i];
        
        sample = DSPUtils::softClip(sample, 2.0f);
        
        const float deEmphasisFreq = 2000.0f;
        const float deEmphasisCoeff = std::exp(-2.0f * kPi * deEmphasisFreq / sampleRate_);
        sample = sample + deEmphasisCoeff * deEmphasisState_;
        deEmphasisState_ = sample;
        
        sample *= masterGain;
        
        sample = DSPUtils::hardClip(sample, 0.95f);
        
        data[i] = sample;
    }
}

void AmpSimulator::setGain(float gainDb) {
    gain_ = (gainDb + 20.0f) / 40.0f;
    gain_ = std::max(0.0f, std::min(1.0f, gain_));
}

void AmpSimulator::setBass(float db) {
    bassDb_ = db;
    toneStack_.setParameters(bassDb_, midDb_, trebleDb_, static_cast<float>(sampleRate_));
}

void AmpSimulator::setMid(float db) {
    midDb_ = db;
    toneStack_.setParameters(bassDb_, midDb_, trebleDb_, static_cast<float>(sampleRate_));
}

void AmpSimulator::setTreble(float db) {
    trebleDb_ = db;
    toneStack_.setParameters(bassDb_, midDb_, trebleDb_, static_cast<float>(sampleRate_));
}

void AmpSimulator::setPresence(float db) {
    float cutoff = 5000.0f + db * 50.0f;
    presenceFilter_.setCutoff(cutoff, static_cast<float>(sampleRate_));
}

void AmpSimulator::setMaster(float db) {
    master_ = (db + 10.0f) / 20.0f;
    master_ = std::max(0.0f, std::min(1.0f, master_));
}

void AmpSimulator::setDrive(float amount) {
    drive_ = std::max(0.0f, std::min(1.0f, amount));
}

void AmpSimulator::ToneStack::setParameters(float bassDb, float midDb, float trebleDb, float sampleRate) {
    bass.setCutoff(100.0f + bassDb * 2.0f, sampleRate);
    mid.setCutoff(1000.0f + midDb * 10.0f, sampleRate);
    treble.setCutoff(5000.0f + trebleDb * 50.0f, sampleRate);
}

float AmpSimulator::ToneStack::process(float input) {
    float output = bass.process(input);
    output = mid.process(output);
    output = treble.process(output);
    return output;
}

void AmpSimulator::ToneStack::reset() {
    bass.reset();
    mid.reset();
    treble.reset();
}

} // namespace openamp
