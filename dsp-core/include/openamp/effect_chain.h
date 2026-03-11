#pragma once

#include "openamp/plugin_interface.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace openamp {

class EffectChain : public AudioProcessor {
public:
    EffectChain();
    ~EffectChain();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Effect Chain"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    void addEffect(AudioProcessorPtr effect);
    void removeEffect(size_t index);
    void clearEffects();
    size_t getNumEffects() const;
    void swapEffects(size_t indexA, size_t indexB);
    
    void setEffectEnabled(size_t index, bool enabled);
    bool isEffectEnabled(size_t index) const;
    
    void setBypass(bool bypass) { bypass_ = bypass; }
    bool isBypassed() const { return bypass_; }

    // Convenience methods for built-in effects
    void setAmpEnabled(bool enabled);
    void setDistortionEnabled(bool enabled);
    void setDelayEnabled(bool enabled);
    void setReverbEnabled(bool enabled);

    void setDistortionType(int type);
    void setDistortionDrive(float drive);
    void setDistortionTone(float tone);
    void setDistortionLevel(float level);

    void setDelayTime(float ms);
    void setDelayFeedback(float feedback);
    void setDelayMix(float mix);
    void setDelayFirst(bool first);

    void setReverbRoom(float room);
    void setReverbDamp(float damp);
    void setReverbMix(float mix);

private:
    struct EffectSlot {
        AudioProcessorPtr processor;
        bool enabled = true;
    };
    
    std::vector<EffectSlot> effects_;
    bool bypass_ = false;
    
    std::vector<float> tempBuffer_;
    uint32_t maxBlockSize_ = 0;
};

} // namespace openamp
