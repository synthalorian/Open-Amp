#include "harmonizer.h"
#include <cmath>
#include <algorithm>

namespace openamp {

Harmonizer::Harmonizer() = default;

void Harmonizer::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    
    // Initialize buffers
    uint32_t bufferSize = std::max(maxBlockSize * 4, kGrainSize * 2u);
    inputBuffer_.resize(bufferSize, 0.0f);
    outputBuffer_.resize(bufferSize, 0.0f);
    writePos_ = 0;
    readPos_ = 0.0f;
    
    // Create crossfade window (raised cosine)
    crossfadeWindow_.resize(512);
    for (size_t i = 0; i < crossfadeWindow_.size(); ++i) {
        float t = static_cast<float>(i) / (crossfadeWindow_.size() - 1);
        crossfadeWindow_[i] = 0.5f * (1.0f - std::cos(t * 3.14159f));
    }
    
    // Initialize grains
    grainA_.fill(0.0f);
    grainB_.fill(0.0f);
    grainWritePos_ = 0;
    grainReadPosA_ = 0.0f;
    grainReadPosB_ = 0.0f;
    grainMix_ = 0.0f;
}

void Harmonizer::reset() {
    std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
    std::fill(outputBuffer_.begin(), outputBuffer_.end(), 0.0f);
    writePos_ = 0;
    readPos_ = 0.0f;
    grainA_.fill(0.0f);
    grainB_.fill(0.0f);
    grainWritePos_ = 0;
    grainReadPosA_ = 0.0f;
    grainReadPosB_ = 0.0f;
    grainMix_ = 0.0f;
}

void Harmonizer::process(AudioBuffer& buffer) {
    if (buffer.numChannels == 0 || buffer.numFrames == 0) return;
    
    // Set shift based on mode
    switch (mode_) {
        case Mode::OctaveUp:
            shiftSemitones_ = 12.0f;
            break;
        case Mode::OctaveDown:
            shiftSemitones_ = -12.0f;
            break;
        case Mode::PerfectFifth:
            shiftSemitones_ = 7.0f;
            break;
        case Mode::OctaveBoth:
            // Process twice with different shifts
            shiftSemitones_ = 12.0f;
            break;
        case Mode::Thirds:
            shiftSemitones_ = 4.0f;
            break;
        case Mode::HarmonyDouble:
            shiftSemitones_ = static_cast<float>(harmonyInterval_);
            break;
    }
    
    for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
        float* channelData = buffer.data + ch * buffer.numFrames;
        pitchShift(channelData, buffer.numFrames);
    }
}

void Harmonizer::pitchShift(float* data, uint32_t numFrames) {
    // Calculate pitch ratio
    float pitchRatio = std::pow(2.0f, shiftSemitones_ / 12.0f);
    
    // Simple granular pitch shifting
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i];
        
        // Store input in circular buffer
        inputBuffer_[writePos_] = input;
        writePos_ = (writePos_ + 1) % inputBuffer_.size();
        
        // Read at different rate for pitch shifting
        float output = readInterpolated(inputBuffer_, readPos_);
        
        // Update read position
        readPos_ += pitchRatio;
        
        // Wrap read position and handle crossfade
        while (readPos_ >= inputBuffer_.size()) {
            readPos_ -= inputBuffer_.size();
        }
        
        // Mix dry/wet
        data[i] = input * (1.0f - mix_) + output * mix_;
    }
}

float Harmonizer::readInterpolated(const std::vector<float>& buffer, float index) {
    if (buffer.empty()) return 0.0f;
    
    uint32_t bufSize = static_cast<uint32_t>(buffer.size());
    uint32_t idx0 = static_cast<uint32_t>(index) % bufSize;
    uint32_t idx1 = (idx0 + 1) % bufSize;
    float frac = index - std::floor(index);
    
    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

void Harmonizer::setMode(Mode mode) {
    mode_ = mode;
}

void Harmonizer::setMix(float amount) {
    mix_ = std::clamp(amount, 0.0f, 1.0f);
}

void Harmonizer::setShiftSemitones(float semitones) {
    shiftSemitones_ = semitones;
}

void Harmonizer::setHarmonyInterval(int semitones) {
    harmonyInterval_ = semitones;
}

void Harmonizer::setTracking(float speed) {
    tracking_ = std::clamp(speed, 0.0f, 1.0f);
}

} // namespace openamp
