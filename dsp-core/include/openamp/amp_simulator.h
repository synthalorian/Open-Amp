#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <array>
#include <cstddef>
#include <vector>
#include <cstdint>
#include <string>

namespace openamp {

class AmpSimulator : public AudioProcessor {
public:
    AmpSimulator();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Amp Simulator"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    void setGain(float gainDb);
    void setBass(float db);
    void setMid(float db);
    void setTreble(float db);
    void setPresence(float db);
    void setMaster(float db);
    void setDrive(float amount);
    void setCabIR(const std::vector<float>& ir);
    
private:
    struct ToneStack {
        OnePoleFilter bass, mid, treble;
        void setParameters(float bassDb, float midDb, float trebleDb, float sampleRate);
        float process(float input);
        void reset();
    };
    
    double sampleRate_ = 48000.0;
    float gain_ = 0.5f;
    float drive_ = 0.5f;
    float master_ = 0.7f;
    float bassDb_ = 0.0f;
    float midDb_ = 0.0f;
    float trebleDb_ = 0.0f;
    
    ToneStack toneStack_;
    OnePoleFilter presenceFilter_;
    OnePoleFilter cabHighPass_;
    OnePoleFilter cabLowPass_;
    std::vector<float> cabIR_;
    std::vector<float> cabHistory_;
    size_t cabIndex_ = 0;
    
    float preEmphasisState_ = 0.0f;
    float deEmphasisState_ = 0.0f;
    
    void processPreamp(float* data, uint32_t numFrames);
    void processPowerAmp(float* data, uint32_t numFrames);
};

} // namespace openamp
