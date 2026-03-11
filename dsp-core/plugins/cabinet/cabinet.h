#pragma once

#include "openamp/plugin_interface.h"
#include "openamp/dsp_utils.h"
#include <vector>
#include <string>
#include <memory>

namespace openamp {

class CabinetSimulator : public AudioProcessor {
public:
    CabinetSimulator();
    ~CabinetSimulator() override;
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Cabinet"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Load impulse response from file
    bool loadIR(const std::string& path);
    
    // Load built-in cabinet model
    enum class CabinetType {
        Custom,         // User-loaded IR
        Marshall4x12,   // Classic British 4x12
        Fender2x12,     // American 2x12 combo
        VoxAC30,        // British 2x12
        MesaBoogie4x12, // High gain 4x12
        Orange4x12,     // Modern British 4x12
        TwinReverb,     // Fender Twin 1x12
        Greenback,      // Celestion Greenback 4x12
        Vintage30,      // Celestion Vintage 30 4x12
        AcousticSim     // Acoustic guitar simulation
    };
    
    void setCabinetType(CabinetType type);
    CabinetType getCabinetType() const { return currentType_; }
    
    // Mix between dry and IR-processed signal
    void setMix(float amount);  // 0.0 = bypass, 1.0 = full
    
    // Pre-IR EQ
    void setLowCut(float hz);
    void setHighCut(float hz);
    
    // Post-IR gain
    void setOutputGain(float db);
    
    bool isIRLoaded() const { return irLoaded_; }
    std::string getIRName() const { return irName_; }
    
private:
    void generateBuiltinIR(CabinetType type);
    void processConvolution(float* data, uint32_t numFrames, uint32_t channels);
    
    double sampleRate_ = 48000.0;
    
    // Impulse response data
    std::vector<float> irData_;
    uint32_t irLength_ = 0;
    bool irLoaded_ = false;
    std::string irName_;
    
    // Convolution state
    std::vector<float> inputBuffer_;
    uint32_t inputWritePos_ = 0;
    
    // Parameters
    CabinetType currentType_ = CabinetType::Marshall4x12;
    float mix_ = 1.0f;
    float lowCutHz_ = 80.0f;
    float highCutHz_ = 8000.0f;
    float outputGainDb_ = 0.0f;
    float outputGainLinear_ = 1.0f;
    
    // Filters
    OnePoleFilter lowCutFilter_;
    OnePoleFilter highCutFilter_;
    
    // Buffer for processing
    std::vector<float> tempBuffer_;
};

} // namespace openamp
