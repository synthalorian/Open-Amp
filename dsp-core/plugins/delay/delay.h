#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <cstdint>
#include <string>
#include <vector>

namespace openamp {

class Delay : public AudioProcessor {
public:
    enum class Mode {
        Normal,
        PingPong,
        Slapback,
        Tape
    };
    
    Delay();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Delay"; }
    std::string getVersion() const override { return "1.1.0"; }
    
    void setTimeMs(float ms);
    void setFeedback(float amount);
    void setMix(float amount);
    void setMode(Mode mode);
    void setHighPass(float hz);
    void setLowPass(float hz);
    
private:
    double sampleRate_ = 48000.0;
    float timeMs_ = 350.0f;
    float feedback_ = 0.35f;
    float mix_ = 0.25f;
    Mode mode_ = Mode::Normal;
    
    float highPassHz_ = 80.0f;
    float lowPassHz_ = 8000.0f;
    
    uint32_t maxDelaySamples_ = 0;
    float delaySamples_ = 0.0f;
    float delaySamplesFrac_ = 0.0f;
    
    std::vector<std::vector<float>> delayLines_;
    std::vector<uint32_t> writeIndices_;
    
    // Filters for feedback path
    std::vector<OnePoleFilter> highPassFilters_;
    std::vector<OnePoleFilter> lowPassFilters_;
    
    // Tape modulation
    float lfoPhase_ = 0.0f;
    float lfoRate_ = 0.5f;  // Hz
    float lfoDepth_ = 0.002f;  // Very subtle
    
    void updateDelaySamples();
    void ensureChannels(uint32_t numChannels);
    float readInterpolated(const std::vector<float>& buffer, uint32_t writeIndex, float delaySamples);
};

} // namespace openamp
