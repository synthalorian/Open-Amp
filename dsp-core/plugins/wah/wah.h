#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <cstdint>
#include <string>

namespace openamp {

class Wah : public AudioProcessor {
public:
    enum class Mode {
        Manual,     // Manual control via position
        Auto,       // LFO-driven auto-wah
        Touch       // Envelope follower touch wah
    };
    
    Wah();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Wah"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Controls
    void setPosition(float position);       // 0 to 1 (heel to toe)
    void setDepth(float depth);             // 0 to 1 (for auto mode)
    void setRate(float hz);                 // LFO rate for auto mode
    void setSensitivity(float sensitivity); // For touch mode
    void setQ(float q);                     // Resonance
    void setMix(float mix);                 // Dry/wet mix
    void setMode(Mode mode);
    void setBypass(bool bypass);
    
    // Getters
    float getPosition() const { return position_; }
    float getDepth() const { return depth_; }
    float getRate() const { return rate_; }
    float getSensitivity() const { return sensitivity_; }
    float getQ() const { return q_; }
    float getMix() const { return mix_; }
    Mode getMode() const { return mode_; }
    bool isBypassed() const { return bypass_; }

private:
    double sampleRate_ = 48000.0;
    
    // Parameters
    float position_ = 0.5f;
    float depth_ = 0.5f;
    float rate_ = 2.0f;
    float sensitivity_ = 0.5f;
    float q_ = 5.0f;
    float mix_ = 1.0f;
    Mode mode_ = Mode::Manual;
    bool bypass_ = false;
    
    // Bandpass filter state (biquad)
    float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f;
    float a1_ = 0.0f, a2_ = 0.0f;
    float x1_ = 0.0f, x2_ = 0.0f;
    float y1_ = 0.0f, y2_ = 0.0f;
    
    // LFO for auto mode
    float lfoPhase_ = 0.0f;
    float lfoIncrement_ = 0.0f;
    
    // Envelope follower for touch mode
    float envelope_ = 0.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    
    void updateFilter(float centerFreq);
    void updateCoefficients();
};

} // namespace openamp
