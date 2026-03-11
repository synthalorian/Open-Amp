#include "acoustic_sim.h"
#include <cmath>
#include <algorithm>

namespace openamp {

AcousticSimulator::AcousticSimulator() = default;

void AcousticSimulator::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    (void)maxBlockSize;
    
    // Setup body resonance filters (simulate acoustic body resonances)
    bodyFilters_.resize(4);
    bodyDelays_.resize(4, 0.0f);
    bodyDelayPos_.resize(4, 0);
    
    // Different delay times for body resonances (based on body size)
    float baseDelays[] = {0.002f, 0.003f, 0.005f, 0.007f};
    for (int i = 0; i < 4; ++i) {
        bodyFilters_[i].setCutoff(500.0f + i * 200.0f, static_cast<float>(sampleRate));
    }
    
    // High shelf for brightness
    highShelf_.setCutoff(3000.0f, static_cast<float>(sampleRate));
    
    // Resonance buffer (short delay for string buzz simulation)
    uint32_t resLength = static_cast<uint32_t>(sampleRate * 0.01);
    resonanceBuffer_.resize(resLength, 0.0f);
    resonancePos_ = 0;
}

void AcousticSimulator::reset() {
    for (auto& filter : bodyFilters_) {
        filter.reset();
    }
    std::fill(bodyDelays_.begin(), bodyDelays_.end(), 0.0f);
    highShelf_.reset();
    std::fill(resonanceBuffer_.begin(), resonanceBuffer_.end(), 0.0f);
    resonancePos_ = 0;
}

void AcousticSimulator::process(AudioBuffer& buffer) {
    if (buffer.numChannels == 0 || buffer.numFrames == 0) return;
    
    for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
        float* channelData = buffer.data + ch * buffer.numFrames;
        
        for (uint32_t i = 0; i < buffer.numFrames; ++i) {
            float dry = channelData[i];
            float wet = dry;
            
            // Add body resonance (comb filter simulation)
            float bodySizeMs = 2.0f + bodySize_ * 8.0f;  // 2-10ms based on body size
            float bodyGain = 0.15f + bodySize_ * 0.2f;
            
            for (int b = 0; b < 4; ++b) {
                float delayMs = bodySizeMs * (1.0f + b * 0.3f);
                uint32_t delaySamples = static_cast<uint32_t>(delayMs * sampleRate_ / 1000.0);
                
                // Simple delay line simulation
                float delayed = bodyDelays_[b];
                bodyDelays_[b] = wet * 0.7f + delayed * 0.3f;
                
                wet += delayed * bodyGain * (1.0f - b * 0.15f);
                wet = bodyFilters_[b].process(wet);
            }
            
            // Add brightness (enhance high frequencies)
            float bright = highShelf_.process(wet);
            wet = wet + (bright - wet) * brightness_;
            
            // Add string resonance/buzz
            uint32_t resIdx = (resonancePos_ + resonanceBuffer_.size() - 
                              static_cast<uint32_t>(sampleRate_ * 0.003)) % resonanceBuffer_.size();
            float resonance = resonanceBuffer_[resIdx];
            resonanceBuffer_[resonancePos_] = wet * 0.5f + resonance * resonanceFeedback_;
            resonancePos_ = (resonancePos_ + 1) % resonanceBuffer_.size();
            
            wet += resonance * resonance_ * 0.3f;
            
            // Mix dry/wet
            channelData[i] = dry * (1.0f - amount_) + wet * amount_;
        }
    }
}

void AcousticSimulator::setAmount(float amount) {
    amount_ = std::clamp(amount, 0.0f, 1.0f);
}

void AcousticSimulator::setBodySize(float size) {
    bodySize_ = std::clamp(size, 0.0f, 1.0f);
}

void AcousticSimulator::setBrightness(float amount) {
    brightness_ = std::clamp(amount, 0.0f, 1.0f);
    highShelf_.setCutoff(2000.0f + brightness_ * 4000.0f, static_cast<float>(sampleRate_));
}

void AcousticSimulator::setResonance(float amount) {
    resonance_ = std::clamp(amount, 0.0f, 1.0f);
    resonanceFeedback_ = resonance_ * 0.4f;
}

} // namespace openamp
