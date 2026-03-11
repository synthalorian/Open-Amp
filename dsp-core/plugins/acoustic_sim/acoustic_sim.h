#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <vector>

namespace openamp {

class AcousticSimulator : public AudioProcessor {
public:
    AcousticSimulator();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Acoustic Sim"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Amount of acoustic simulation (0.0 = bypass, 1.0 = full)
    void setAmount(float amount);
    
    // Body size simulation (0.0 = small, 1.0 = jumbo)
    void setBodySize(float size);
    
    // Brightness enhancement
    void setBrightness(float amount);
    
    // String resonance simulation
    void setResonance(float amount);

private:
    void processSample(float& sample);
    
    double sampleRate_ = 48000.0;
    float amount_ = 0.5f;
    float bodySize_ = 0.5f;
    float brightness_ = 0.5f;
    float resonance_ = 0.3f;
    
    // Body resonance filters (multiple delays for body simulation)
    std::vector<OnePoleFilter> bodyFilters_;
    std::vector<float> bodyDelays_;
    std::vector<uint32_t> bodyDelayPos_;
    
    // High frequency enhancer
    OnePoleFilter highShelf_;
    
    // String resonance
    std::vector<float> resonanceBuffer_;
    uint32_t resonancePos_ = 0;
    float resonanceFeedback_ = 0.0f;
};

} // namespace openamp
