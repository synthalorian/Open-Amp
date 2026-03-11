#pragma once

#include "openamp/plugin_interface.h"
#include <cstdint>
#include <string>
#include <functional>

namespace openamp {

class Metronome : public AudioProcessor {
public:
    enum class TimeSignature {
        FourFour,    // 4/4
        ThreeFour,   // 3/4
        SixEight,    // 6/8
        TwoFour,     // 2/4
        FiveFour,    // 5/4
        Custom       // Custom beats per measure
    };
    
    struct BeatInfo {
        int currentBeat;        // Current beat (1-based)
        int beatsPerMeasure;    // Total beats in measure
        bool isDownbeat;        // Is this the first beat?
        float position;         // Position in measure (0-1)
    };
    
    using BeatCallback = std::function<void(const BeatInfo&)>;
    
    Metronome();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Metronome"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Transport
    void start();
    void stop();
    bool isPlaying() const { return playing_; }
    
    // Tempo
    void setTempo(float bpm);
    float getTempo() const { return tempo_; }
    
    // Time signature
    void setTimeSignature(TimeSignature sig);
    TimeSignature getTimeSignature() const { return timeSignature_; }
    void setCustomBeats(int beats);
    int getBeatsPerMeasure() const { return beatsPerMeasure_; }
    
    // Sound settings
    void setClickLevel(float level);       // 0 to 1
    float getClickLevel() const { return clickLevel_; }
    
    void setHighPitch(float hz);           // Downbeat pitch
    void setLowPitch(float hz);            // Other beats pitch
    float getHighPitch() const { return highPitch_; }
    float getLowPitch() const { return lowPitch_; }
    
    void setClickDuration(float ms);
    float getClickDuration() const { return clickDurationMs_; }
    
    // Output mode
    void setMixWithAudio(bool mix);
    bool isMixingWithAudio() const { return mixWithAudio_; }
    
    // Callback for visual sync
    void setBeatCallback(BeatCallback callback);
    
    // Get current beat info
    BeatInfo getBeatInfo() const;

private:
    double sampleRate_ = 48000.0;
    
    bool playing_ = false;
    float tempo_ = 120.0f;
    TimeSignature timeSignature_ = TimeSignature::FourFour;
    int beatsPerMeasure_ = 4;
    
    // Click sound
    float clickLevel_ = 0.5f;
    float highPitch_ = 1000.0f;   // Downbeat
    float lowPitch_ = 800.0f;     // Other beats
    float clickDurationMs_ = 10.0f;
    bool mixWithAudio_ = true;
    
    // Internal state
    uint64_t samplesPerBeat_ = 0;
    uint64_t sampleCounter_ = 0;
    int currentBeat_ = 1;
    
    // Click generator
    float clickPhase_ = 0.0f;
    int clickSamplesRemaining_ = 0;
    float currentClickPitch_ = 800.0f;
    
    // Callback
    BeatCallback beatCallback_;
    
    void updateSamplesPerBeat();
    float generateClick();
};

} // namespace openamp
