#include "modulation.h"
#include <cmath>
#include <algorithm>
#include <array>

namespace openamp {

// LFO implementation
void LFO::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    phaseIncrement_ = frequency_ / static_cast<float>(sampleRate_);
}

void LFO::setFrequency(float hz) {
    frequency_ = hz;
    phaseIncrement_ = frequency_ / static_cast<float>(sampleRate_);
}

void LFO::setWaveform(Waveform waveform) {
    waveform_ = waveform;
}

void LFO::setPhase(float phase) {
    phase_ = phase;
}

void LFO::reset() {
    phase_ = 0.0f;
    lastValue_ = 0.0f;
}

float LFO::process() {
    constexpr float kPi = 3.14159265358979323846f;
    
    switch (waveform_) {
        case Waveform::Sine:
            lastValue_ = std::sin(2.0f * kPi * phase_);
            break;
        case Waveform::Triangle:
            lastValue_ = 4.0f * std::abs(phase_ - 0.5f) - 1.0f;
            break;
        case Waveform::Square:
            lastValue_ = phase_ < 0.5f ? 1.0f : -1.0f;
            break;
        case Waveform::SawUp:
            lastValue_ = 2.0f * phase_ - 1.0f;
            break;
        case Waveform::SawDown:
            lastValue_ = 1.0f - 2.0f * phase_;
            break;
    }
    
    phase_ += phaseIncrement_;
    while (phase_ >= 1.0f) phase_ -= 1.0f;
    
    return lastValue_;
}

// DelayLine implementation
void DelayLine::setSize(size_t size) {
    size_ = size;
    buffer_.resize(size, 0.0f);
    writeIndex_ = 0;
}

void DelayLine::reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writeIndex_ = 0;
}

float DelayLine::read(float delaySamples) const {
    // Linear interpolation
    float readPos = static_cast<float>(writeIndex_) - delaySamples;
    while (readPos < 0.0f) readPos += static_cast<float>(size_);
    while (readPos >= static_cast<float>(size_)) readPos -= static_cast<float>(size_);
    
    size_t index0 = static_cast<size_t>(readPos);
    size_t index1 = (index0 + 1) % size_;
    float frac = readPos - static_cast<float>(index0);
    
    return buffer_[index0] * (1.0f - frac) + buffer_[index1] * frac;
}

void DelayLine::write(float sample) {
    buffer_[writeIndex_] = sample;
    writeIndex_ = (writeIndex_ + 1) % size_;
}

// Modulation implementation
Modulation::Modulation() {
    // Initialize chorus
    chorusDelays_.resize(8);
    chorusLFOs_.resize(8);
    
    // Initialize phaser filters
    phaserFilters_.resize(12);
    for (auto& f : phaserFilters_) {
        f.fill(0.0f);
    }
}

void Modulation::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    
    // Setup chorus delays
    size_t maxChorusSamples = static_cast<size_t>(50.0f * sampleRate_ / 1000.0f);
    for (auto& delay : chorusDelays_) {
        delay.setSize(maxChorusSamples);
    }
    
    // Setup chorus LFOs with phase offsets
    for (size_t i = 0; i < chorusLFOs_.size(); ++i) {
        chorusLFOs_[i].prepare(sampleRate);
        chorusLFOs_[i].setFrequency(rate_);
        chorusLFOs_[i].setPhase(static_cast<float>(i) / static_cast<float>(chorusLFOs_.size()));
    }
    
    // Setup flanger
    size_t maxFlangerSamples = static_cast<size_t>(20.0f * sampleRate_ / 1000.0f);
    flangerDelay_.setSize(maxFlangerSamples);
    flangerLFO_.prepare(sampleRate);
    flangerLFO_.setFrequency(rate_);
    
    // Setup phaser
    phaserLFO_.prepare(sampleRate);
    phaserLFO_.setFrequency(rate_);
    
    // Setup tremolo
    tremoloLFO_.prepare(sampleRate);
    tremoloLFO_.setFrequency(rate_);
    tremoloLFO_.setWaveform(tremoloWaveform_);
    
    updateCoefficients();
}

void Modulation::updateCoefficients() {
    for (auto& lfo : chorusLFOs_) {
        lfo.setFrequency(rate_);
    }
    flangerLFO_.setFrequency(rate_);
    phaserLFO_.setFrequency(rate_);
    tremoloLFO_.setFrequency(rate_);
}

void Modulation::reset() {
    for (auto& delay : chorusDelays_) {
        delay.reset();
    }
    for (auto& lfo : chorusLFOs_) {
        lfo.reset();
    }
    flangerDelay_.reset();
    flangerLFO_.reset();
    phaserLFO_.reset();
    tremoloLFO_.reset();
    flangerLast_ = 0.0f;
    phaserLast_ = 0.0f;
    for (auto& f : phaserFilters_) {
        f.fill(0.0f);
    }
}

void Modulation::process(AudioBuffer& buffer) {
    if (bypass_) return;
    
    switch (type_) {
        case Type::Chorus:
            processChorus(buffer.data, buffer.numFrames);
            break;
        case Type::Flanger:
            processFlanger(buffer.data, buffer.numFrames);
            break;
        case Type::Phaser:
            processPhaser(buffer.data, buffer.numFrames);
            break;
        case Type::Tremolo:
            processTremolo(buffer.data, buffer.numFrames);
            break;
        case Type::Vibrato:
            processVibrato(buffer.data, buffer.numFrames);
            break;
    }
}

void Modulation::processChorus(float* data, uint32_t numFrames) {
    float baseDelaySamples = chorusDelayMs_ * static_cast<float>(sampleRate_) / 1000.0f;
    float modulationRange = 5.0f * static_cast<float>(sampleRate_) / 1000.0f;  // 5ms modulation range
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i];
        float output = input;
        
        // Process each chorus voice
        for (int v = 0; v < chorusVoices_; ++v) {
            float lfo = chorusLFOs_[v].process();
            float delaySamples = baseDelaySamples + lfo * depth_ * modulationRange;
            float delayed = chorusDelays_[v].read(delaySamples);
            output += delayed / static_cast<float>(chorusVoices_ + 1);
            chorusDelays_[v].write(input);
        }
        
        // Mix
        data[i] = input * (1.0f - mix_) + output * mix_;
    }
}

void Modulation::processFlanger(float* data, uint32_t numFrames) {
    float baseDelaySamples = flangerDelayMs_ * static_cast<float>(sampleRate_) / 1000.0f;
    float modulationRange = baseDelaySamples * 2.0f;  // Sweep up to 2x the base delay
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i];
        
        // LFO modulation
        float lfo = flangerLFO_.process();
        float delaySamples = baseDelaySamples + (lfo + 1.0f) * 0.5f * depth_ * modulationRange;
        
        // Read delayed sample with feedback
        float delayed = flangerDelay_.read(delaySamples);
        
        // Write to delay with feedback
        flangerDelay_.write(input + delayed * flangerFeedback_);
        
        // Mix dry and wet
        float output = input + delayed * mix_;
        
        data[i] = output;
    }
}

void Modulation::processPhaser(float* data, uint32_t numFrames) {
    constexpr float kPi = 3.14159265358979323846f;
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i] + phaserLast_ * phaserFeedback_;
        float output = input;
        
        // LFO for frequency modulation
        float lfo = phaserLFO_.process();
        
        // Process all-pass filter stages
        for (int s = 0; s < phaserStages_; ++s) {
            // Modulate frequency per stage with spread
            float stageFreq = 500.0f + 2000.0f * (lfo * 0.5f + 0.5f) * depth_;
            stageFreq *= std::pow(phaserSpread_, static_cast<float>(s) - static_cast<float>(phaserStages_) / 2.0f);
            
            // Compute all-pass filter coefficient
            float omega = 2.0f * kPi * stageFreq / static_cast<float>(sampleRate_);
            float tanw = std::tan(omega / 2.0f);
            float c = (tanw - 1.0f) / (tanw + 1.0f);
            
            // All-pass filter: y = c*x + x1 - c*y1
            auto& f = phaserFilters_[s];
            float x = output;
            float y = c * x + f[0] - c * f[2];
            f[0] = x;
            f[2] = y;
            
            output = y;
        }
        
        phaserLast_ = output;
        
        // Mix dry and wet
        data[i] = data[i] * (1.0f - mix_) + output * mix_;
    }
}

void Modulation::processTremolo(float* data, uint32_t numFrames) {
    for (uint32_t i = 0; i < numFrames; ++i) {
        float lfo = tremoloLFO_.process();
        float amplitude = 1.0f - depth_ * (1.0f - (lfo + 1.0f) * 0.5f);
        data[i] *= amplitude;
    }
}

void Modulation::processVibrato(float* data, uint32_t numFrames) {
    DelayLine vibratoDelay;
    float maxDelayMs = 10.0f;
    size_t maxDelaySamples = static_cast<size_t>(maxDelayMs * sampleRate_ / 1000.0f) + 1;
    vibratoDelay.setSize(maxDelaySamples);
    
    float baseDelaySamples = 5.0f * static_cast<float>(sampleRate_) / 1000.0f;
    float modulationRange = 5.0f * static_cast<float>(sampleRate_) / 1000.0f;
    
    LFO vibratoLFO;
    vibratoLFO.prepare(sampleRate_);
    vibratoLFO.setFrequency(rate_);
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i];
        
        float lfo = vibratoLFO.process();
        float delaySamples = baseDelaySamples + lfo * depth_ * modulationRange;
        
        vibratoDelay.write(input);
        float output = vibratoDelay.read(delaySamples);
        
        data[i] = output;
    }
}

void Modulation::setType(Type type) {
    type_ = type;
    reset();
}

void Modulation::setRate(float hz) {
    rate_ = std::clamp(hz, 0.1f, 20.0f);
    updateCoefficients();
}

void Modulation::setDepth(float depth) {
    depth_ = std::clamp(depth, 0.0f, 1.0f);
}

void Modulation::setMix(float mix) {
    mix_ = std::clamp(mix, 0.0f, 1.0f);
}

void Modulation::setBypass(bool bypass) {
    bypass_ = bypass;
}

void Modulation::setChorusVoices(int voices) {
    chorusVoices_ = std::clamp(voices, 1, 8);
}

void Modulation::setChorusDelay(float ms) {
    chorusDelayMs_ = std::clamp(ms, 5.0f, 30.0f);
}

void Modulation::setFlangerFeedback(float fb) {
    flangerFeedback_ = std::clamp(fb, 0.0f, 0.95f);
}

void Modulation::setFlangerDelay(float ms) {
    flangerDelayMs_ = std::clamp(ms, 0.1f, 10.0f);
}

void Modulation::setPhaserStages(int stages) {
    phaserStages_ = std::clamp(stages, 2, 12);
    phaserFilters_.resize(phaserStages_);
    for (auto& f : phaserFilters_) {
        f.fill(0.0f);
    }
}

void Modulation::setPhaserFeedback(float fb) {
    phaserFeedback_ = std::clamp(fb, 0.0f, 0.95f);
}

void Modulation::setPhaserSpread(float spread) {
    phaserSpread_ = std::clamp(spread, 0.5f, 2.0f);
}

void Modulation::setTremoloWaveform(LFO::Waveform waveform) {
    tremoloWaveform_ = waveform;
    tremoloLFO_.setWaveform(waveform);
}

} // namespace openamp
