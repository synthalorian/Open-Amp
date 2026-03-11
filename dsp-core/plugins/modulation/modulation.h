#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace openamp {

// LFO for modulation effects
class LFO {
public:
    enum class Waveform {
        Sine,
        Triangle,
        Square,
        SawUp,
        SawDown
    };
    
    LFO() = default;
    
    void prepare(double sampleRate);
    void setFrequency(float hz);
    void setWaveform(Waveform waveform);
    void setPhase(float phase);  // 0 to 1
    void reset();
    
    float process();
    float getValue() const { return lastValue_; }
    
private:
    double sampleRate_ = 48000.0;
    float frequency_ = 1.0f;
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    Waveform waveform_ = Waveform::Sine;
    float lastValue_ = 0.0f;
};

// Delay line for modulation effects
class DelayLine {
public:
    void setSize(size_t size);
    void reset();
    float read(float delaySamples) const;
    void write(float sample);
    float readLast() const { return buffer_[writeIndex_]; }
    
private:
    std::vector<float> buffer_;
    size_t writeIndex_ = 0;
    size_t size_ = 0;
};

class Modulation : public AudioProcessor {
public:
    enum class Type {
        Chorus,
        Flanger,
        Phaser,
        Tremolo,
        Vibrato
    };
    
    Modulation();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Modulation"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Common controls
    void setType(Type type);
    void setRate(float hz);              // 0.1 to 20 Hz
    void setDepth(float depth);          // 0 to 1
    void setMix(float mix);              // 0 to 1
    void setBypass(bool bypass);
    
    // Chorus controls
    void setChorusVoices(int voices);    // 1 to 8
    void setChorusDelay(float ms);       // 5 to 30 ms
    
    // Flanger controls
    void setFlangerFeedback(float fb);   // 0 to 0.95
    void setFlangerDelay(float ms);      // 0.1 to 10 ms
    
    // Phaser controls
    void setPhaserStages(int stages);    // 2 to 12
    void setPhaserFeedback(float fb);    // 0 to 0.95
    void setPhaserSpread(float spread);  // 0.5 to 2.0
    
    // Tremolo controls
    void setTremoloWaveform(LFO::Waveform waveform);
    
    // Getters
    Type getType() const { return type_; }
    float getRate() const { return rate_; }
    float getDepth() const { return depth_; }
    float getMix() const { return mix_; }
    bool isBypassed() const { return bypass_; }
    int getChorusVoices() const { return chorusVoices_; }
    float getChorusDelay() const { return chorusDelayMs_; }
    float getFlangerFeedback() const { return flangerFeedback_; }
    float getFlangerDelay() const { return flangerDelayMs_; }
    int getPhaserStages() const { return phaserStages_; }
    float getPhaserFeedback() const { return phaserFeedback_; }

private:
    double sampleRate_ = 48000.0;
    
    Type type_ = Type::Chorus;
    float rate_ = 1.5f;
    float depth_ = 0.5f;
    float mix_ = 0.5f;
    bool bypass_ = false;
    
    // Chorus
    int chorusVoices_ = 4;
    float chorusDelayMs_ = 15.0f;
    std::vector<DelayLine> chorusDelays_;
    std::vector<LFO> chorusLFOs_;
    
    // Flanger
    float flangerFeedback_ = 0.7f;
    float flangerDelayMs_ = 2.0f;
    DelayLine flangerDelay_;
    LFO flangerLFO_;
    float flangerLast_ = 0.0f;
    
    // Phaser
    int phaserStages_ = 4;
    float phaserFeedback_ = 0.7f;
    float phaserSpread_ = 1.0f;
    LFO phaserLFO_;
    std::vector<std::array<float, 4>> phaserFilters_;  // [stages][x1, x2, y1, y2]
    float phaserLast_ = 0.0f;
    
    // Tremolo
    LFO tremoloLFO_;
    LFO::Waveform tremoloWaveform_ = LFO::Waveform::Sine;
    
    void processChorus(float* data, uint32_t numFrames);
    void processFlanger(float* data, uint32_t numFrames);
    void processPhaser(float* data, uint32_t numFrames);
    void processTremolo(float* data, uint32_t numFrames);
    void processVibrato(float* data, uint32_t numFrames);
    
    void updateCoefficients();
};

} // namespace openamp
