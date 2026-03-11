#pragma once

#ifdef USE_ALSA

#include "audio_backend.h"
#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>
#include <chrono>

namespace openamp {

class ALSABackend : public AudioBackend {
public:
    ALSABackend();
    ~ALSABackend() override;

    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override;

    std::vector<AudioDevice> getInputDevices() override;
    std::vector<AudioDevice> getOutputDevices() override;
    AudioDevice getDefaultInputDevice() override;
    AudioDevice getDefaultOutputDevice() override;

    bool setConfig(const AudioConfig& config) override;
    AudioConfig getConfig() const override;

    bool start(AudioCallback callback) override;
    void stop() override;
    bool isRunning() const override;

    double getInputLatency() const override;
    double getOutputLatency() const override;

    // Metering
    MeterLevels getInputLevels() const override;
    MeterLevels getOutputLevels() const override;
    void resetPeakHold() override;

    std::string getName() const override { return "ALSA"; }
    std::string getVersion() const override;
    bool isAvailable() const override { return true; }

    std::string getLastError() const override { return lastError_; }

private:
    void audioThread();
    bool setupPCM(snd_pcm_t** pcm, const std::string& device, bool isInput);
    void scanDevices();
    
    // Helper to calculate dB from linear
    static float linearToDb(float linear);
    void updateMeters(const float* input, const float* output, uint32_t numFrames);

    snd_pcm_t* inputPCM_ = nullptr;
    snd_pcm_t* outputPCM_ = nullptr;

    AudioConfig config_;
    AudioCallback callback_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    std::string lastError_;

    std::vector<AudioDevice> inputDevices_;
    std::vector<AudioDevice> outputDevices_;

    std::thread audioThread_;
    double inputLatency_ = 0.0;
    double outputLatency_ = 0.0;
    
    // Metering (atomic for thread-safe access)
    std::atomic<float> inputLevelLeft_{-60.0f};
    std::atomic<float> inputLevelRight_{-60.0f};
    std::atomic<float> inputPeakLeft_{-60.0f};
    std::atomic<float> inputPeakRight_{-60.0f};
    std::atomic<float> outputLevelLeft_{-60.0f};
    std::atomic<float> outputLevelRight_{-60.0f};
    std::atomic<float> outputPeakLeft_{-60.0f};
    std::atomic<float> outputPeakRight_{-60.0f};
    
    // Peak hold decay timer
    std::chrono::steady_clock::time_point lastPeakUpdate_;
    static constexpr float PEAK_HOLD_SECONDS = 2.0f;
    static constexpr float PEAK_DECAY_DB_PER_SEC = 10.0f;
};

} // namespace openamp

#endif // USE_ALSA
