#include "metronome.h"
#include <cmath>
#include <algorithm>

namespace openamp {

Metronome::Metronome() {
    updateSamplesPerBeat();
}

void Metronome::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    updateSamplesPerBeat();
    reset();
}

void Metronome::reset() {
    sampleCounter_ = 0;
    currentBeat_ = 1;
    clickPhase_ = 0.0f;
    clickSamplesRemaining_ = 0;
}

void Metronome::updateSamplesPerBeat() {
    // BPM to samples per beat
    samplesPerBeat_ = static_cast<uint64_t>((60.0 / tempo_) * sampleRate_);
}

void Metronome::process(AudioBuffer& buffer) {
    if (!playing_) {
        if (!mixWithAudio_) {
            // Mute output when not playing in non-mix mode
            for (uint32_t i = 0; i < buffer.numFrames; ++i) {
                buffer.data[i] = 0.0f;
            }
        }
        return;
    }
    
    float* data = buffer.data;
    uint32_t numFrames = buffer.numFrames;
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float click = 0.0f;
        
        // Check if we need to trigger a click
        if (sampleCounter_ == 0) {
            // New beat
            clickSamplesRemaining_ = static_cast<int>((clickDurationMs_ / 1000.0f) * sampleRate_);
            
            if (currentBeat_ == 1) {
                currentClickPitch_ = highPitch_;
            } else {
                currentClickPitch_ = lowPitch_;
            }
            
            // Fire callback
            if (beatCallback_) {
                BeatInfo info;
                info.currentBeat = currentBeat_;
                info.beatsPerMeasure = beatsPerMeasure_;
                info.isDownbeat = (currentBeat_ == 1);
                info.position = 0.0f;
                beatCallback_(info);
            }
        }
        
        // Generate click sound
        if (clickSamplesRemaining_ > 0) {
            click = generateClick();
            clickSamplesRemaining_--;
        }
        
        // Mix click with audio or replace
        if (mixWithAudio_) {
            data[i] = data[i] * 0.8f + click * 0.2f;
        } else {
            data[i] = click;
        }
        
        // Advance counter
        sampleCounter_++;
        if (sampleCounter_ >= samplesPerBeat_) {
            sampleCounter_ = 0;
            currentBeat_++;
            if (currentBeat_ > beatsPerMeasure_) {
                currentBeat_ = 1;
            }
        }
    }
}

float Metronome::generateClick() {
    // Simple sine wave click with envelope
    constexpr float kPi = 3.14159265358979323846f;
    
    float phaseIncrement = (2.0f * kPi * currentClickPitch_) / static_cast<float>(sampleRate_);
    clickPhase_ += phaseIncrement;
    if (clickPhase_ >= 2.0f * kPi) clickPhase_ -= 2.0f * kPi;
    
    float sample = std::sin(clickPhase_);
    
    // Apply envelope (quick decay)
    int totalSamples = static_cast<int>((clickDurationMs_ / 1000.0f) * sampleRate_);
    float envelope = static_cast<float>(clickSamplesRemaining_) / static_cast<float>(totalSamples);
    envelope = envelope * envelope;  // Exponential decay
    
    return sample * envelope * clickLevel_;
}

void Metronome::start() {
    playing_ = true;
    sampleCounter_ = 0;
    currentBeat_ = 1;
}

void Metronome::stop() {
    playing_ = false;
}

void Metronome::setTempo(float bpm) {
    tempo_ = std::clamp(bpm, 20.0f, 300.0f);
    updateSamplesPerBeat();
}

void Metronome::setTimeSignature(TimeSignature sig) {
    timeSignature_ = sig;
    switch (sig) {
        case TimeSignature::FourFour:
            beatsPerMeasure_ = 4;
            break;
        case TimeSignature::ThreeFour:
            beatsPerMeasure_ = 3;
            break;
        case TimeSignature::SixEight:
            beatsPerMeasure_ = 6;
            break;
        case TimeSignature::TwoFour:
            beatsPerMeasure_ = 2;
            break;
        case TimeSignature::FiveFour:
            beatsPerMeasure_ = 5;
            break;
        case TimeSignature::Custom:
            // Keep current custom value
            break;
    }
}

void Metronome::setCustomBeats(int beats) {
    beatsPerMeasure_ = std::clamp(beats, 1, 16);
    timeSignature_ = TimeSignature::Custom;
}

void Metronome::setClickLevel(float level) {
    clickLevel_ = std::clamp(level, 0.0f, 1.0f);
}

void Metronome::setHighPitch(float hz) {
    highPitch_ = std::clamp(hz, 200.0f, 2000.0f);
}

void Metronome::setLowPitch(float hz) {
    lowPitch_ = std::clamp(hz, 100.0f, 1500.0f);
}

void Metronome::setClickDuration(float ms) {
    clickDurationMs_ = std::clamp(ms, 5.0f, 100.0f);
}

void Metronome::setMixWithAudio(bool mix) {
    mixWithAudio_ = mix;
}

void Metronome::setBeatCallback(BeatCallback callback) {
    beatCallback_ = callback;
}

Metronome::BeatInfo Metronome::getBeatInfo() const {
    BeatInfo info;
    info.currentBeat = currentBeat_;
    info.beatsPerMeasure = beatsPerMeasure_;
    info.isDownbeat = (currentBeat_ == 1);
    info.position = static_cast<float>(sampleCounter_) / static_cast<float>(samplesPerBeat_);
    return info;
}

} // namespace openamp
