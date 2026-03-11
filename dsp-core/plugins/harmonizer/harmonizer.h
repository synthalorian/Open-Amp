#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <vector>
#include <array>

namespace openamp {

class Harmonizer : public AudioProcessor {
public:
    Harmonizer();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Harmonizer"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    enum class Mode {
        OctaveUp,       // +12 semitones
        OctaveDown,     // -12 semitones
        PerfectFifth,   // +7 semitones
        OctaveBoth,     // +12 and -12 together
        Thirds,         // +4 semitones (major third)
        HarmonyDouble   // Intelligent harmony
    };
    
    void setMode(Mode mode);
    Mode getMode() const { return mode_; }
    
    // Dry/wet mix
    void setMix(float amount);
    
    // Pitch shift amount in semitones (for custom mode)
    void setShiftSemitones(float semitones);
    
    // Harmony interval for HarmonyDouble mode
    void setHarmonyInterval(int semitones);
    
    // Tracking speed (lower = faster but glitchier)
    void setTracking(float speed);

private:
    void pitchShift(float* data, uint32_t numFrames);
    float readInterpolated(const std::vector<float>& buffer, float index);
    
    double sampleRate_ = 48000.0;
    Mode mode_ = Mode::OctaveDown;
    float mix_ = 0.5f;
    float shiftSemitones_ = -12.0f;
    int harmonyInterval_ = 7;  // Perfect fifth
    float tracking_ = 0.5f;
    
    // Pitch shifter state
    std::vector<float> inputBuffer_;
    std::vector<float> outputBuffer_;
    uint32_t writePos_ = 0;
    float readPos_ = 0.0f;
    float crossfadePos_ = 0.0f;
    bool crossfading_ = false;
    
    // Window for crossfade
    std::vector<float> crossfadeWindow_;
    
    // Pitch detection (simplified)
    float lastPeriod_ = 0.0f;
    float detectedPitch_ = 0.0f;
    
    // Grain buffers for PSOLA
    static constexpr uint32_t kGrainSize = 2048;
    std::array<float, kGrainSize> grainA_;
    std::array<float, kGrainSize> grainB_;
    uint32_t grainWritePos_ = 0;
    float grainReadPosA_ = 0.0f;
    float grainReadPosB_ = 0.0f;
    float grainMix_ = 0.0f;
};

} // namespace openamp
