#include "openamp/amp_simulator.h"
#include "openamp/dsp_constants.h"
#include "openamp/simd_utils.h"
#include <algorithm>
#include <cmath>

namespace openamp {

using namespace constants;

AmpSimulator::AmpSimulator() = default;

void AmpSimulator::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    reset();
    
    presenceFilter_.setCutoff(kPresenceBaseHz, static_cast<float>(sampleRate));
    cabHighPass_.setCutoff(kCabHighPassHz, static_cast<float>(sampleRate));
    cabLowPass_.setCutoff(kCabLowPassHz, static_cast<float>(sampleRate));
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
    const float preGain = DSPUtils::dbToGain(gain_ * kGainRangeDb + kGainOffsetDb);
    const float driveAmount = kDriveOffset + drive_ * kDriveScale;
    const float emphasisCoeff = std::exp(-kTwoPi * kPreEmphasisHz / static_cast<float>(sampleRate_));

    // SIMD pass 1: apply pre-gain
    simd::apply_gain(data, numFrames, preGain);

    // SIMD pass 2: tanh saturation
    simd::soft_clip_buffer(data, numFrames, driveAmount);

    // Scalar pass: stateful pre-emphasis filter (cannot vectorize due to feedback)
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = data[i] - emphasisCoeff * preEmphasisState_;
        preEmphasisState_ = sample;
        data[i] = sample;
    }
}

void AmpSimulator::processPowerAmp(float* data, uint32_t numFrames) {
    const float masterGain = DSPUtils::dbToGain(master_ * kMasterRangeDb + kMasterOffsetDb);
    const float deEmphasisCoeff = std::exp(-kTwoPi * kDeEmphasisHz / static_cast<float>(sampleRate_));

    // SIMD pass 1: soft clip
    simd::soft_clip_buffer(data, numFrames, kPowerAmpSoftClipDrive);

    // Scalar pass: stateful de-emphasis + master gain + hard clip
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = data[i] + deEmphasisCoeff * deEmphasisState_;
        deEmphasisState_ = sample;
        sample *= masterGain;
        sample = DSPUtils::hardClip(sample, kPowerAmpHardClipThreshold);
        data[i] = sample;
    }
}

void AmpSimulator::setGain(float gainDb) {
    gain_ = (gainDb - kGainOffsetDb) / kGainRangeDb;
    gain_ = std::clamp(gain_, 0.0f, 1.0f);
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
    float cutoff = kPresenceBaseHz + db * kPresenceScalePerDb;
    presenceFilter_.setCutoff(cutoff, static_cast<float>(sampleRate_));
}

void AmpSimulator::setMaster(float db) {
    master_ = (db - kMasterOffsetDb) / kMasterRangeDb;
    master_ = std::clamp(master_, 0.0f, 1.0f);
}

void AmpSimulator::setDrive(float amount) {
    drive_ = std::clamp(amount, 0.0f, 1.0f);
}

void AmpSimulator::ToneStack::setParameters(float bassDb, float midDb, float trebleDb, float sampleRate) {
    bass.setCutoff(kBassBaseHz + bassDb * kBassScalePerDb, sampleRate);
    mid.setCutoff(kMidBaseHz + midDb * kMidScalePerDb, sampleRate);
    treble.setCutoff(kTrebleBaseHz + trebleDb * kTrebleScalePerDb, sampleRate);
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
