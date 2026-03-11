#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <cstdint>
#include <string>
#include <vector>

namespace openamp {

class Reverb : public AudioProcessor {
public:
    enum class Type {
        Hall,
        Room,
        Plate,
        Spring
    };
    
    Reverb();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Reverb"; }
    std::string getVersion() const override { return "1.1.0"; }
    
    void setRoomSize(float amount);
    void setDamping(float amount);
    void setMix(float amount);
    void setPreDelay(float ms);
    void setWidth(float amount);
    void setType(Type type);
    
private:
    struct Comb {
        std::vector<float> buffer;
        uint32_t index = 0;
        float feedback = 0.5f;
        float damp = 0.2f;
        float filterStore = 0.0f;
        
        void resize(uint32_t size);
        float process(float input);
        void reset();
    };
    
    struct Allpass {
        std::vector<float> buffer;
        uint32_t index = 0;
        float feedback = 0.5f;
        
        void resize(uint32_t size);
        float process(float input);
        void reset();
    };
    
    struct DelayLine {
        std::vector<float> buffer;
        uint32_t index = 0;
        
        void resize(uint32_t size);
        void write(float sample);
        float read(uint32_t delay) const;
        void reset();
    };
    
    double sampleRate_ = 48000.0;
    float roomSize_ = 0.5f;
    float damping_ = 0.3f;
    float mix_ = 0.25f;
    float preDelayMs_ = 20.0f;
    float width_ = 1.0f;
    Type type_ = Type::Hall;
    
    // Main reverb (Freeverb-style)
    std::vector<Comb> combsL_;
    std::vector<Comb> combsR_;
    std::vector<Allpass> allpassesL_;
    std::vector<Allpass> allpassesR_;
    
    // Pre-delay
    DelayLine preDelay_;
    
    // Early reflections (simple tapped delay)
    DelayLine earlyRef_;
    std::vector<uint32_t> earlyTapTimes_;
    std::vector<float> earlyTapGains_;
    
    void updateParameters();
    void ensureInitialized();
    void processSpring(float* data, uint32_t numFrames);
};

} // namespace openamp
