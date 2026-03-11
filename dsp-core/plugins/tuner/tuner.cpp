#include "tuner.h"
#include <cmath>
#include <algorithm>
#include <complex>

namespace openamp {

Tuner::Tuner() {
    buffer_.fill(0.0f);
    lastResult_.valid = false;
    lastResult_.frequency = 0.0f;
    lastResult_.cents = 0.0f;
    lastResult_.noteIndex = 0;
    lastResult_.confidence = 0.0f;
}

void Tuner::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void Tuner::reset() {
    buffer_.fill(0.0f);
    writeIndex_ = 0;
    bufferFull_ = false;
    inputLevel_ = 0.0f;
    lastResult_.valid = false;
}

void Tuner::process(AudioBuffer& buffer) {
    float* data = buffer.data;
    uint32_t numFrames = buffer.numFrames;
    
    // Accumulate input level
    float level = 0.0f;
    for (uint32_t i = 0; i < numFrames; ++i) {
        float absVal = std::abs(data[i]);
        if (absVal > level) level = absVal;
        
        // Store in circular buffer
        buffer_[writeIndex_] = data[i];
        writeIndex_ = (writeIndex_ + 1) % kBufferSize;
    }
    
    if (writeIndex_ == 0) bufferFull_ = true;
    
    // Smooth level meter
    inputLevel_ = inputLevel_ * 0.9f + level * 0.1f;
    
    // Detect frequency if we have enough signal
    if (level > 0.01f && bufferFull_) {
        float freq = detectFrequency();
        if (freq > 0.0f) {
            lastResult_ = frequencyToResult(freq);
        }
    } else {
        lastResult_.valid = false;
    }
    
    // Mute output if enabled
    if (mute_) {
        for (uint32_t i = 0; i < numFrames; ++i) {
            data[i] = 0.0f;
        }
    }
}

float Tuner::detectFrequency() {
    // Autocorrelation-based pitch detection
    float bestCorrelation = -1.0f;
    int bestLag = 0;
    
    // Calculate autocorrelation for each lag
    for (int lag = kMinLag; lag < kMaxLag; ++lag) {
        float correlation = 0.0f;
        float energy = 0.0f;
        
        for (int i = 0; i < kBufferSize - lag; ++i) {
            int idx1 = (writeIndex_ + i) % kBufferSize;
            int idx2 = (writeIndex_ + i + lag) % kBufferSize;
            correlation += buffer_[idx1] * buffer_[idx2];
            energy += buffer_[idx1] * buffer_[idx1];
        }
        
        if (energy > 0.0001f) {
            correlation /= energy;
            
            // Find first peak (skip the zero-lag peak)
            if (correlation > bestCorrelation) {
                bestCorrelation = correlation;
                bestLag = lag;
            }
        }
    }
    
    // If we found a good correlation, calculate frequency
    if (bestCorrelation > 0.5f && bestLag > 0) {
        float frequency = static_cast<float>(sampleRate_) / static_cast<float>(bestLag);
        lastResult_.confidence = bestCorrelation;
        return frequency;
    }
    
    lastResult_.confidence = 0.0f;
    return 0.0f;
}

Tuner::DetectionResult Tuner::frequencyToResult(float freq) {
    DetectionResult result;
    result.frequency = freq;
    result.valid = true;
    result.confidence = lastResult_.confidence;
    
    // Convert frequency to semitone (A4 = semitone 69)
    int semitone = frequencyToSemitone(freq);
    
    // Get note name
    result.noteName = semitoneToName(semitone);
    result.noteIndex = semitone % 12;
    
    // Calculate cents deviation from nearest note
    float nearestNoteFreq = noteToFrequency(semitone);
    float cents = 1200.0f * std::log2(freq / nearestNoteFreq);
    result.cents = cents;
    
    return result;
}

int Tuner::frequencyToSemitone(float freq) {
    // A4 (440 Hz) = MIDI note 69
    float semitone = 69.0f + 12.0f * std::log2(freq / referencePitch_);
    return static_cast<int>(std::round(semitone));
}

float Tuner::noteToFrequency(int semitone) {
    return referencePitch_ * std::pow(2.0f, (semitone - 69) / 12.0f);
}

std::string Tuner::semitoneToName(int semitone) {
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (semitone / 12) - 1;
    int note = semitone % 12;
    return std::string(noteNames[note]) + std::to_string(octave);
}

void Tuner::setReferencePitch(float hz) {
    referencePitch_ = std::clamp(hz, 420.0f, 460.0f);
}

void Tuner::setMode(Mode mode) {
    mode_ = mode;
}

void Tuner::setMute(bool mute) {
    mute_ = mute;
}

} // namespace openamp
