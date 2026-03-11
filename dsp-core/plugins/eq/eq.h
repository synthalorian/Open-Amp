#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <cstdint>
#include <string>
#include <array>

namespace openamp {

// Biquad filter for parametric EQ bands
class BiquadFilter {
public:
    enum class Type {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        Peaking,
        LowShelf,
        HighShelf
    };
    
    BiquadFilter() = default;
    
    void setType(Type type) { type_ = type; }
    void setFrequency(float freqHz) { freq_ = freqHz; }
    void setQ(float q) { q_ = q; }
    void setGain(float gainDb) { gainDb_ = gainDb; }
    
    void prepare(double sampleRate);
    float process(float input);
    void reset();
    
private:
    Type type_ = Type::Peaking;
    double sampleRate_ = 48000.0;
    float freq_ = 1000.0f;
    float q_ = 0.707f;
    float gainDb_ = 0.0f;
    
    // Coefficients
    float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f;
    float a1_ = 0.0f, a2_ = 0.0f;
    
    // State
    float x1_ = 0.0f, x2_ = 0.0f;
    float y1_ = 0.0f, y2_ = 0.0f;
    
    void updateCoefficients();
};

class EQ : public AudioProcessor {
public:
    static constexpr size_t kNumBands = 10;
    
    struct Band {
        BiquadFilter filter;
        bool enabled = true;
        float frequency = 1000.0f;
        float gain = 0.0f;     // -24 to +24 dB
        float q = 1.0f;        // 0.1 to 10
        BiquadFilter::Type type = BiquadFilter::Type::Peaking;
    };
    
    EQ();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "EQ"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Band controls
    void setBandEnabled(size_t band, bool enabled);
    void setBandFrequency(size_t band, float freqHz);
    void setBandGain(size_t band, float gainDb);
    void setBandQ(size_t band, float q);
    void setBandType(size_t band, BiquadFilter::Type type);
    
    bool getBandEnabled(size_t band) const;
    float getBandFrequency(size_t band) const;
    float getBandGain(size_t band) const;
    float getBandQ(size_t band) const;
    BiquadFilter::Type getBandType(size_t band) const;
    
    // Preset bands
    void setLowCut(float freqHz, float slope = 12.0f);
    void setHighCut(float freqHz, float slope = 12.0f);
    
    // Master
    void setOutputGain(float gainDb);
    void setBypass(bool bypass);
    
    size_t getNumBands() const { return kNumBands; }
    
private:
    std::array<Band, kNumBands> bands_;
    double sampleRate_ = 48000.0;
    float outputGainDb_ = 0.0f;
    bool bypass_ = false;
};

} // namespace openamp
