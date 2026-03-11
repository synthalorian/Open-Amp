#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <cstdint>
#include <string>

namespace openamp {

class Compressor : public AudioProcessor {
public:
    enum class Mode {
        HardKnee,
        SoftKnee,
        Limiter
    };
    
    enum class Detector {
        Peak,
        RMS,
        Feedback
    };
    
    Compressor();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Compressor"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Controls
    void setThreshold(float thresholdDb);   // -60 to 0 dB
    void setRatio(float ratio);              // 1:1 to 20:1
    void setAttack(float attackMs);          // 0.1 to 100 ms
    void setRelease(float releaseMs);        // 10 to 1000 ms
    void setKnee(float kneeDb);              // 0 to 24 dB (soft knee width)
    void setMakeupGain(float gainDb);        // 0 to 24 dB
    void setMix(float mix);                  // 0 to 1 (dry/wet)
    void setMode(Mode mode);
    void setDetector(Detector detector);
    void setBypass(bool bypass);
    
    // Getters
    float getThreshold() const { return thresholdDb_; }
    float getRatio() const { return ratio_; }
    float getAttack() const { return attackMs_; }
    float getRelease() const { return releaseMs_; }
    float getKnee() const { return kneeDb_; }
    float getMakeupGain() const { return makeupGainDb_; }
    float getMix() const { return mix_; }
    Mode getMode() const { return mode_; }
    Detector getDetector() const { return detector_; }
    bool isBypassed() const { return bypass_; }
    
    // Metering
    float getGainReduction() const { return gainReductionDb_; }
    float getInputLevel() const { return inputLevelDb_; }
    float getOutputLevel() const { return outputLevelDb_; }

private:
    double sampleRate_ = 48000.0;
    
    // Parameters
    float thresholdDb_ = -20.0f;
    float ratio_ = 4.0f;
    float attackMs_ = 10.0f;
    float releaseMs_ = 100.0f;
    float kneeDb_ = 6.0f;
    float makeupGainDb_ = 0.0f;
    float mix_ = 1.0f;
    Mode mode_ = Mode::SoftKnee;
    Detector detector_ = Detector::RMS;
    bool bypass_ = false;
    
    // Envelope follower state
    float envelope_ = 0.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    
    // RMS detector
    static constexpr int kRmsWindowSize = 64;
    float rmsBuffer_[64] = {0.0f};
    int rmsIndex_ = 0;
    float rmsSum_ = 0.0f;
    
    // Metering
    float gainReductionDb_ = 0.0f;
    float inputLevelDb_ = -60.0f;
    float outputLevelDb_ = -60.0f;
    float meterSmoothing_ = 0.0f;
    float meterSmoothingOut_ = 0.0f;
    
    void updateCoefficients();
    float computeGainReduction(float inputDb);
    float detectLevel(float* data, uint32_t numFrames);
};

} // namespace openamp
