#pragma once

#include "openamp/amp_simulator.h"
#include "openamp/effect_chain.h"
#include "openamp/preset_store.h"
#include "openamp/ir_loader.h"
#include "openamp/latency_monitor.h"
#include <memory>
#include <string>

// Forward declarations for built-in effects
namespace openamp {
    class NoiseGate;
    class Compressor;
    class EQ;
    class Distortion;
    class Delay;
    class Reverb;
}

namespace openamp {

class DSPEngine {
public:
    DSPEngine();
    ~DSPEngine();

    void prepare(double sampleRate, uint32_t blockSize);
    void process(float* data, uint32_t numFrames);
    void reset();

    // Amp controls
    void setAmpEnabled(bool enabled);
    void setGain(float gainDb);
    void setDrive(float drive);
    void setBass(float db);
    void setMid(float db);
    void setTreble(float db);
    void setPresence(float db);
    void setMaster(float db);

    // Noise gate
    void setNoiseGateEnabled(bool enabled);
    void setNoiseGateThreshold(float db);
    void setNoiseGateAttack(float ms);
    void setNoiseGateRelease(float ms);

    // Compressor
    void setCompressorEnabled(bool enabled);
    void setCompressorThreshold(float db);
    void setCompressorRatio(float ratio);
    void setCompressorAttack(float ms);
    void setCompressorRelease(float ms);

    // EQ
    void setEQEnabled(bool enabled);
    void setEQBand(int band, float db);

    // Effect toggles
    void setDistortionEnabled(bool enabled);
    void setDelayEnabled(bool enabled);
    void setReverbEnabled(bool enabled);

    // Distortion controls
    void setDistortionType(int type);
    void setDistortionDrive(float drive);
    void setDistortionTone(float tone);
    void setDistortionLevel(float level);

    // Delay controls
    void setDelayTime(float ms);
    void setDelayFeedback(float feedback);
    void setDelayMix(float mix);
    void setDelayFirst(bool first);

    // Reverb controls
    void setReverbRoom(float room);
    void setReverbDamp(float damp);
    void setReverbMix(float mix);

    // IR Loader controls
    bool loadIR(const std::string& path, std::string& error);
    void setIREnabled(bool enabled);
    void setIRMix(float mix);
    void setIRInputGain(float gainDb);
    void setIROutputGain(float gainDb);
    void setIRHighCut(float hz);
    void setIRLowCut(float hz);
    std::string getIRName() const;
    float getIRCPUsage() const;

    // Latency monitoring
    float getLatencyMs() const;
    float getTheoreticalLatencyMs() const;
    uint32_t getBufferSize() const { return blockSize_; }

    // Presets
    bool loadPreset(const std::string& path, std::string& error);
    bool savePreset(const std::string& path, std::string& error);
    void applyPreset(const Preset& preset);
    const Preset& getCurrentPreset() const { return currentPreset_; }

    double getSampleRate() const { return sampleRate_; }
    uint32_t getBlockSize() const { return blockSize_; }

private:
    double sampleRate_ = 48000.0;
    uint32_t blockSize_ = 256;

    // Amp
    std::unique_ptr<AmpSimulator> amp_;
    bool ampEnabled_ = true;

    // Effects (owned directly for simplicity)
    std::unique_ptr<NoiseGate> noiseGate_;
    std::unique_ptr<Compressor> compressor_;
    std::unique_ptr<EQ> eq_;
    std::unique_ptr<Distortion> distortion_;
    std::unique_ptr<Delay> delay_;
    std::unique_ptr<Reverb> reverb_;

    // Effect enable states
    bool noiseGateEnabled_ = true;
    bool compressorEnabled_ = false;
    bool eqEnabled_ = false;
    bool distortionEnabled_ = false;
    bool delayEnabled_ = true;
    bool reverbEnabled_ = true;
    bool delayFirst_ = true;

    // IR Loader
    std::unique_ptr<IRLoader> irLoader_;
    bool irEnabled_ = false;

    // Latency monitoring
    LatencyMonitor latencyMonitor_;

    Preset currentPreset_;
};

} // namespace openamp
