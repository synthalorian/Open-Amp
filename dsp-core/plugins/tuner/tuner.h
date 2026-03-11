#pragma once

#include "openamp/plugin_interface.h"
#include <cstdint>
#include <string>
#include <array>

namespace openamp {

class Tuner : public AudioProcessor {
public:
    // Standard guitar tuning frequencies (E2, A2, D3, G3, B3, E4)
    static constexpr float kStandardTuning[] = {82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f};
    static constexpr const char* kStandardNotes[] = {"E2", "A2", "D3", "G3", "B3", "E4"};
    
    struct DetectionResult {
        float frequency;        // Detected frequency in Hz
        float cents;            // Cents sharp (+) or flat (-)
        int noteIndex;          // Closest note index (0-5 for standard)
        std::string noteName;   // Note name (e.g., "A2")
        float confidence;       // Detection confidence 0-1
        bool valid;             // Is detection valid?
    };
    
    Tuner();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Tuner"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Get the latest detection result
    DetectionResult getLastDetection() const { return lastResult_; }
    
    // Set reference pitch (default A4 = 440 Hz)
    void setReferencePitch(float hz);
    float getReferencePitch() const { return referencePitch_; }
    
    // Set detection mode
    enum class Mode {
        Chromatic,      // All notes
        GuitarStandard, // Standard guitar tuning only
        BassStandard    // Standard bass tuning
    };
    void setMode(Mode mode);
    Mode getMode() const { return mode_; }
    
    // Enable/disable mute while tuning
    void setMute(bool mute);
    bool isMuted() const { return mute_; }
    
    // Get level for meter display
    float getInputLevel() const { return inputLevel_; }

private:
    double sampleRate_ = 48000.0;
    float referencePitch_ = 440.0f;
    Mode mode_ = Mode::Chromatic;
    bool mute_ = true;
    
    DetectionResult lastResult_;
    float inputLevel_ = 0.0f;
    
    // Autocorrelation buffer
    static constexpr int kBufferSize = 2048;
    std::array<float, kBufferSize> buffer_;
    int writeIndex_ = 0;
    bool bufferFull_ = false;
    
    // Note detection
    static constexpr int kMinLag = 32;      // Corresponds to ~1500 Hz
    static constexpr int kMaxLag = 1024;    // Corresponds to ~47 Hz
    
    float detectFrequency();
    DetectionResult frequencyToResult(float freq);
    float noteToFrequency(int semitone);
    int frequencyToSemitone(float freq);
    std::string semitoneToName(int semitone);
};

} // namespace openamp
