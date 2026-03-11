#pragma once

#ifdef USE_PIPEWIRE

#include "audio_backend.h"
#include <pipewire/pipewire.h>
#include <pipewire/extensions/metadata.h>
#include <spa/param/audio/format-utils.h>
#include <spa/utils/keys.h>
#include <spa/utils/hook.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace openamp {

class PipeWireBackend : public AudioBackend {
public:
    PipeWireBackend();
    ~PipeWireBackend() override;

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

    std::string getName() const override { return "PipeWire"; }
    std::string getVersion() const override;
    bool isAvailable() const override { return true; }

    std::string getLastError() const override { return lastError_; }

    // Public callbacks for PipeWire events
    static void onRegistryGlobal(void* data, uint32_t id, uint32_t permissions,
                                  const char* type, uint32_t version,
                                  const struct spa_dict* props);
    static void onRegistryGlobalRemove(void* data, uint32_t id);
    static int onMetadataProperty(void* data, uint32_t subject, const char* key,
                                    const char* type, const char* value);

private:
    static void onInputProcess(void* userdata);
    static void onOutputProcess(void* userdata);
    void handleInputProcess();
    void handleOutputProcess();
    
    // Helper to calculate dB from linear
    static float linearToDb(float linear);
    void updateMeters(const float* input, const float* output, uint32_t numFrames);

    pw_main_loop* mainLoop_ = nullptr;
    pw_context* context_ = nullptr;
    pw_core* core_ = nullptr;
    pw_registry* registry_ = nullptr;
    pw_metadata* metadata_ = nullptr;
    pw_stream* inputStream_ = nullptr;
    pw_stream* outputStream_ = nullptr;

    AudioConfig config_;
    AudioCallback callback_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    std::string lastError_;

    struct DeviceInfo {
        uint32_t id;
        AudioDevice device;
    };
    std::vector<DeviceInfo> inputDevices_;
    std::vector<DeviceInfo> outputDevices_;
    std::string defaultInputName_;
    std::string defaultOutputName_;
    std::mutex devicesMutex_;

    std::thread loopThread_;
    std::condition_variable initCv_;
    bool devicesReady_ = false;

    uint32_t inputNodeId_ = PW_ID_ANY;
    uint32_t outputNodeId_ = PW_ID_ANY;
    
    // Hooks for listeners
    spa_hook registryListener_;
    spa_hook metadataListener_;
    spa_hook inputStreamListener_;
    spa_hook outputStreamListener_;
    
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

#endif // USE_PIPEWIRE
