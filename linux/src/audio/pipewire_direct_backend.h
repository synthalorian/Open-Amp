#pragma once

#include "audio_backend.h"
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

namespace openamp {

class PipeWireDirectBackend : public AudioBackend {
public:
    PipeWireDirectBackend();
    virtual ~PipeWireDirectBackend();

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

    MeterLevels getInputLevels() const override;
    MeterLevels getOutputLevels() const override;
    void resetPeakHold() override;

    std::string getName() const override { return "PipeWire"; }
    std::string getVersion() const override; 
    std::string getLastError() const override { return lastError_; }
    bool isAvailable() const override { return true; }

private:
    static void onCoreInfo(void* data, const struct pw_core_info* info);
    static void onRegistryGlobal(void* data, uint32_t id, uint32_t permissions,
                                  const char* type, uint32_t version,
                                  const struct spa_dict* props);
    static void onStreamProcess(void* data);
    static void onStreamStateChanged(void* data, enum pw_stream_state old_state,
                                      enum pw_stream_state new_state, const char* error);

    void scanDevices();
    
    pw_main_loop* mainLoop_ = nullptr;
    pw_context* context_ = nullptr;
    pw_core* core_ = nullptr;
    pw_registry* registry_ = nullptr;
    pw_stream* inputStream_ = nullptr;
    pw_stream* outputStream_ = nullptr;
    struct spa_hook listener_; // ADDED THIS

    AudioCallback callback_;
    AudioConfig config_;
    bool initialized_ = false;
    std::atomic<bool> running_{false};
    std::string lastError_;
    std::thread loopThread_;

    mutable std::mutex devicesMutex_;
    std::vector<AudioDevice> inputDevices_;
    std::vector<AudioDevice> outputDevices_;

    // Metering
    std::atomic<float> inputLevel_{-60.0f};
    std::atomic<float> outputLevelLeft_{-60.0f};
    std::atomic<float> outputLevelRight_{-60.0f};

    float linearToDb(float linear);
};

} // namespace openamp
