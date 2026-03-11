#include "openamp/effect_chain.h"

namespace openamp {

EffectChain::EffectChain() = default;
EffectChain::~EffectChain() = default;

void EffectChain::prepare(double sampleRate, uint32_t maxBlockSize) {
    maxBlockSize_ = maxBlockSize;
    tempBuffer_.resize(maxBlockSize);
    
    for (auto& slot : effects_) {
        if (slot.processor) {
            slot.processor->prepare(sampleRate, maxBlockSize);
        }
    }
}

void EffectChain::process(AudioBuffer& buffer) {
    if (bypass_ || effects_.empty()) return;
    
    for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
        float* channelData = buffer.data + ch * buffer.numFrames;
        
        for (auto& slot : effects_) {
            if (!slot.enabled || !slot.processor) continue;
            
            AudioBuffer effectBuffer;
            effectBuffer.data = channelData;
            effectBuffer.numChannels = 1;
            effectBuffer.numFrames = buffer.numFrames;
            effectBuffer.sampleRate = buffer.sampleRate;
            
            slot.processor->process(effectBuffer);
        }
    }
}

void EffectChain::reset() {
    for (auto& slot : effects_) {
        if (slot.processor) {
            slot.processor->reset();
        }
    }
}

void EffectChain::addEffect(AudioProcessorPtr effect) {
    if (effect) {
        effects_.push_back({std::move(effect), true});
    }
}

void EffectChain::removeEffect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void EffectChain::clearEffects() {
    effects_.clear();
}

size_t EffectChain::getNumEffects() const {
    return effects_.size();
}

void EffectChain::swapEffects(size_t indexA, size_t indexB) {
    if (indexA >= effects_.size() || indexB >= effects_.size()) return;
    if (indexA == indexB) return;
    std::swap(effects_[indexA], effects_[indexB]);
}

void EffectChain::setEffectEnabled(size_t index, bool enabled) {
    if (index < effects_.size()) {
        effects_[index].enabled = enabled;
    }
}

bool EffectChain::isEffectEnabled(size_t index) const {
    if (index < effects_.size()) {
        return effects_[index].enabled;
    }
    return false;
}

} // namespace openamp
