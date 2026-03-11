#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <cstdint>
#include <string>

namespace openamp {

class Distortion : public AudioProcessor {
public:
    enum class Type {
        Overdrive,
        Fuzz,
        Tube,
        HardClip
    };
    
    Distortion();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Distortion"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    void setDrive(float amount);
    void setTone(float amount);
    void setLevel(float amount);
    void setType(Type type);
    
private:
    Type type_ = Type::Overdrive;
    float drive_ = 0.5f;
    float tone_ = 0.5f;
    float level_ = 0.7f;
    
    OnePoleFilter toneFilter_;
    double sampleRate_ = 48000.0;
    
    float processSample(float input);
    void processOverdriveSIMD(float* data, uint32_t numFrames);
};

} // namespace openamp
