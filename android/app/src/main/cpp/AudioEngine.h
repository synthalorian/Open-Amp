#pragma once

#include <oboe/Oboe.h>
#include "openamp/input_processor.h"
#include "openamp/amp_simulator.h"
#include "openamp/effect_chain.h"
#include "openamp/preset_store.h"
#include "openamp/ir_loader.h"
#include "openamp/latency_monitor.h"
#include "delay.h"
#include "reverb.h"
#include "distortion.h"
#include "noise_gate/noise_gate.h"
#include "cabinet.h"
#include "acoustic_sim.h"
#include "harmonizer.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <vector>

namespace openamp_android {

class AudioEngine : public oboe::AudioStreamCallback {
public:
    AudioEngine();
    ~AudioEngine() override;

    // Transport
    bool start();
    void stop();
    bool isRunning() const { return running_.load(); }

    // Device management
    void setInputDeviceId(int32_t deviceId);
    void setOutputDeviceId(int32_t deviceId);

    // Gain controls
    void setInputGain(float db);
    void setOutputGain(float db);
    float getInputGainDb() const;
    float getOutputGainDb() const;

    // Amp controls
    void setAmpEnabled(bool enabled);
    void setAmpGainDb(float db);
    void setAmpDrive(float amount);
    void setAmpBassDb(float db);
    void setAmpMidDb(float db);
    void setAmpTrebleDb(float db);
    void setAmpPresenceDb(float db);
    void setAmpMasterDb(float db);

    bool getAmpEnabled() const;
    float getAmpGainDb() const;
    float getAmpDrive() const;
    float getAmpBassDb() const;
    float getAmpMidDb() const;
    float getAmpTrebleDb() const;
    float getAmpPresenceDb() const;
    float getAmpMasterDb() const;

    // Effects master
    void setEffectsEnabled(bool enabled);
    bool getEffectsEnabled() const;

    // Noise Gate
    void setNoiseGateEnabled(bool enabled);
    void setNoiseGateThreshold(float db);
    void setNoiseGateAttack(float ms);
    void setNoiseGateRelease(float ms);
    bool getNoiseGateEnabled() const;
    float getNoiseGateThreshold() const;

    // Distortion
    void setDistortionEnabled(bool enabled);
    void setDistortionType(int type);
    void setDistortionDrive(float amount);
    void setDistortionTone(float amount);
    void setDistortionLevel(float amount);

    bool getDistortionEnabled() const;
    int getDistortionType() const;
    float getDistortionDrive() const;
    float getDistortionTone() const;
    float getDistortionLevel() const;

    // Delay
    void setDelayEnabled(bool enabled);
    void setDelayTimeMs(float ms);
    void setDelayFeedback(float amount);
    void setDelayMix(float amount);

    bool getDelayEnabled() const;
    float getDelayTimeMs() const;
    float getDelayFeedback() const;
    float getDelayMix() const;

    // Reverb
    void setReverbEnabled(bool enabled);
    void setReverbRoomSize(float amount);
    void setReverbDamping(float amount);
    void setReverbMix(float amount);

    bool getReverbEnabled() const;
    float getReverbRoom() const;
    float getReverbDamp() const;
    float getReverbMix() const;

    // Effect order
    void setDelayFirst(bool enabled);
    bool getDelayFirst() const;

    // IR Cabinet Loader (NEW)
    bool loadIRFromWavFile(const std::string& path);
    void setIREnabled(bool enabled);
    void setIRMix(float mix);
    void setIRInputGain(float db);
    void setIROutputGain(float db);
    void setIRHighCut(float hz);
    void setIRLowCut(float hz);
    bool isIRLoaded() const;
    std::string getIRName() const;
    float getIRCPUsage() const;
    bool getIREnabled() const;
    float getIRMix() const;
    
    // Latency Monitoring (NEW)
    float getLatencyMs() const;
    float getTheoreticalLatencyMs() const;
    uint32_t getBufferSize() const { return config_.bufferSize; }
    double getSampleRate() const { return config_.sampleRate; }

    // Cabinet IR (legacy)
    bool setCabIRFromFile(const std::string& path);
    const std::string& getCabIrPath() const { return cabIrPath_; }
    
    // Cabinet
    void setCabinetEnabled(bool enabled);
    void setCabinetType(int type);
    void setCabinetMix(float amount);
    bool getCabinetEnabled() const;
    int getCabinetType() const;
    float getCabinetMix() const;
    
    // Acoustic Simulator
    void setAcousticSimEnabled(bool enabled);
    void setAcousticAmount(float amount);
    void setAcousticBodySize(float size);
    void setAcousticBrightness(float amount);
    bool getAcousticSimEnabled() const;
    float getAcousticAmount() const;
    
    // Harmonizer
    void setHarmonizerEnabled(bool enabled);
    void setHarmonizerMode(int mode);
    void setHarmonizerMix(float amount);
    bool getHarmonizerEnabled() const;
    int getHarmonizerMode() const;
    float getHarmonizerMix() const;

    // Presets
    bool savePreset(const std::string& path, const std::string& name);
    bool loadPreset(const std::string& path);

    // Meters
    float getInputLevel() const;
    float getOutputLevel() const;
    bool getClipping() const;
    void resetClipIndicator();
    std::string getDebugStatus() const;
    void setInputChannelMode(int mode); // 0=L/Mono, 1=R, 2=Sum, 3=Auto strongest
    int getInputChannelMode() const { return inputChannelMode_; }
    void setTestToneEnabled(bool enabled);
    bool getTestToneEnabled() const { return testToneEnabled_; }
    float getRawInputLevel() const { return rawInputLevel_.load(); }

    // Oboe callbacks
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream* audioStream,
        void* audioData,
        int32_t numFrames) override;

    void onErrorBeforeClose(oboe::AudioStream* audioStream, oboe::Result error) override;
    void onErrorAfterClose(oboe::AudioStream* audioStream, oboe::Result error) override;

private:
    // Audio streams
    std::shared_ptr<oboe::AudioStream> inputStream_;
    std::shared_ptr<oboe::AudioStream> outputStream_;

    oboe::AudioStreamBuilder inputBuilder_;
    oboe::AudioStreamBuilder outputBuilder_;

    int32_t inputDeviceId_ = -1;
    int32_t outputDeviceId_ = -1;

    // DSP components
    std::unique_ptr<openamp::InputProcessor> processor_;
    openamp::AmpSimulator* amp_ = nullptr;
    openamp::EffectChain* chain_ = nullptr;
    openamp::NoiseGate* noiseGate_ = nullptr;
    openamp::Distortion* distortion_ = nullptr;
    openamp::Delay* delay_ = nullptr;
    openamp::Reverb* reverb_ = nullptr;
    openamp::CabinetSimulator* cabinet_ = nullptr;
    openamp::AcousticSimulator* acousticSim_ = nullptr;
    openamp::Harmonizer* harmonizer_ = nullptr;
    
    // NEW: IR Loader
    std::unique_ptr<openamp::IRLoader> irLoader_;
    bool irEnabled_ = false;
    float irMix_ = 1.0f;
    
    // NEW: Latency Monitor
    openamp::LatencyMonitor latencyMonitor_;

    // Processing config
    openamp::ProcessingConfig config_;

    // Effect states
    bool distortionEnabled_ = false;
    bool noiseGateEnabled_ = true;
    bool delayEnabled_ = true;
    bool reverbEnabled_ = true;
    bool delayFirst_ = true;
    bool cabinetEnabled_ = true;
    bool acousticSimEnabled_ = false;
    bool harmonizerEnabled_ = false;

    // Effect parameters
    float ampGainDb_ = 0.0f;
    float ampDrive_ = 0.5f;
    float ampBassDb_ = 0.0f;
    float ampMidDb_ = 0.0f;
    float ampTrebleDb_ = 0.0f;
    float ampPresenceDb_ = 0.0f;
    float ampMasterDb_ = 0.0f;
    float noiseGateThreshold_ = -45.0f;
    float noiseGateAttack_ = 1.0f;
    float noiseGateRelease_ = 100.0f;
    float distortionDrive_ = 0.5f;
    float distortionTone_ = 0.5f;
    float distortionLevel_ = 0.7f;
    int distortionType_ = 0;
    float delayTimeMs_ = 350.0f;
    float delayFeedback_ = 0.35f;
    float delayMix_ = 0.25f;
    float reverbRoom_ = 0.5f;
    float reverbDamp_ = 0.3f;
    float reverbMix_ = 0.25f;
    
    // New effects parameters
    int cabinetType_ = 0;
    float cabinetMix_ = 1.0f;
    float acousticAmount_ = 0.5f;
    float acousticBodySize_ = 0.5f;
    float acousticBrightness_ = 0.5f;
    int harmonizerMode_ = 0;
    float harmonizerMix_ = 0.5f;

    std::string cabIrPath_;

    // Buffers
    std::vector<float> inputBuffer_;          // mono buffer into DSP
    std::vector<float> inputInterleavedBuffer_; // raw input from stream (possibly multi-channel)
    std::vector<float> outputBuffer_;

    // Debug counters/state
    std::atomic<uint64_t> callbackCount_{0};
    std::atomic<uint64_t> inputReadCount_{0};
    std::atomic<uint64_t> inputTimeoutCount_{0};
    std::atomic<uint64_t> inputErrorCount_{0};
    std::atomic<int32_t> lastFramesRequested_{0};
    std::atomic<int32_t> lastFramesRead_{0};
    std::atomic<float> rawInputLevel_{0.0f};

    int inputChannelMode_ = 3; // 0=L/Mono, 1=R, 2=Sum, 3=Auto strongest
    bool testToneEnabled_ = false;
    float testTonePhase_ = 0.0f;

    // State
    std::atomic<bool> running_{false};
    std::atomic<bool> restartRequested_{false};
    std::mutex restartMutex_;

    // Internal methods
    bool openStreams();
    void closeStreams();
    void applyPreset(const openamp::Preset& preset);
};

} // namespace openamp_android
