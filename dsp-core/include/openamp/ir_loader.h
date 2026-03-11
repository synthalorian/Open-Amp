#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <vector>
#include <string>
#include <memory>
#include <chrono>

namespace openamp {

/**
 * IR Cabinet Loader - Convolution reverb using Impulse Responses
 * 
 * Simulates guitar cabinets and acoustic spaces by convolving
 * the input signal with an impulse response (IR) file.
 */
class IRLoader : public AudioProcessor {
public:
    IRLoader();
    ~IRLoader() override;
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "IR Cabinet Loader"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // IR file loading
    bool loadIR(const std::string& path, std::string& errorMessage);
    bool isIRLoaded() const { return !ir_.empty(); }
    
    // Controls
    void setMix(float mix);
    void setInputGain(float gainDb);
    void setOutputGain(float gainDb);
    void setEnabled(bool enabled);
    void setHighCut(float hz);
    void setLowCut(float hz);
    
    // Info
    std::string getIRName() const { return irName_; }
    size_t getIRLength() const { return ir_.size(); }
    double getIRSampleRate() const { return irSampleRate_; }
    float getCurrentCPU() const { return cpuUsage_; }
    
private:
    // Parameters
    float mix_ = 1.0f;
    float inputGain_ = 1.0f;
    float outputGain_ = 1.0f;
    bool enabled_ = true;
    float highCutHz_ = 16000.0f;
    float lowCutHz_ = 80.0f;
    
    // IR data
    std::vector<float> ir_;
    std::string irName_;
    size_t irLength_ = 0;
    double irSampleRate_ = 48000.0;
    
    // Processing state
    double sampleRate_ = 48000.0;
    uint32_t maxBlockSize_ = 256;
    bool prepared_ = false;
    
    // Convolution buffers
    std::vector<float> historyBuffer_;
    size_t historyIndex_ = 0;
    
    // Filters
    OnePoleFilter highCut_;
    OnePoleFilter lowCut_;
    
    // CPU monitoring
    float cpuUsage_ = 0.0f;
    uint64_t processTimeUs_ = 0;
    uint64_t processCount_ = 0;
    
    // WAV loading
    struct WAVHeader {
        uint32_t sampleRate;
        uint16_t numChannels;
        uint16_t bitsPerSample;
        uint32_t dataSize;
    };
    
    bool parseWAV(const std::string& filePath, std::vector<float>& samples,
                  WAVHeader& header, std::string& errorMessage);
    void resampleIR(const std::vector<float>& input, double inputRate,
                    std::vector<float>& output, double outputRate);
    void trimIR(std::vector<float>& ir);
    void normalizeIR(std::vector<float>& ir);
};

} // namespace openamp
