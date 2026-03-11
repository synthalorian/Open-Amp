#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <cstdint>
#include <string>

namespace openamp {

class NoiseGate : public AudioProcessor {
public:
    enum class Mode {
        Hard,       // Instant on/off at threshold
        Soft,       // Smooth transition
        Lookahead   // Uses lookahead to anticipate transients
    };
    
    NoiseGate();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Noise Gate"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Controls
    void setThreshold(float thresholdDb);   // -80 to 0 dB
    void setAttack(float attackMs);          // 0.1 to 50 ms
    void setHold(float holdMs);              // 0 to 500 ms
    void setRelease(float releaseMs);        // 10 to 1000 ms
    void setRange(float rangeDb);            // 0 to -80 dB (how much to attenuate when closed)
    void setMode(Mode mode);
    void setBypass(bool bypass);
    
    // Getters
    float getThreshold() const { return thresholdDb_; }
    float getAttack() const { return attackMs_; }
    float getHold() const { return holdMs_; }
    float getRelease() const { return releaseMs_; }
    float getRange() const { return rangeDb_; }
    Mode getMode() const { return mode_; }
    bool isBypassed() const { return bypass_; }
    
    // Metering
    float getGainReduction() const { return gainReductionDb_; }
    bool isOpen() const { return isOpen_; }

private:
    double sampleRate_ = 48000.0;
    
    // Parameters
    float thresholdDb_ = -40.0f;
    float attackMs_ = 1.0f;
    float holdMs_ = 50.0f;
    float releaseMs_ = 100.0f;
    float rangeDb_ = -40.0f;
    Mode mode_ = Mode::Soft;
    bool bypass_ = false;
    
    // Envelope state
    float envelope_ = 0.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    
    // Hold timer
    int holdSamples_ = 0;
    int holdCounter_ = 0;
    
    // Gate state
    float currentGain_ = 1.0f;
    float targetGain_ = 1.0f;
    bool isOpen_ = true;
    
    // Metering
    float gainReductionDb_ = 0.0f;
    
    // Lookahead buffer
    static constexpr int kLookaheadSamples = 64;
    float lookaheadBuffer_[kLookaheadSamples] = {0.0f};
    int lookaheadIndex_ = 0;
    
    void updateCoefficients();
};

} // namespace openamp
