#pragma once

#include "audio_backend.h"
#include <jack/jack.h>
#include <atomic>
#include <thread>
#include <mutex>

namespace openamp {

class JackBackend : public AudioBackend {
public:
    JackBackend();
    virtual ~JackBackend();

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

    std::string getName() const override { return "JACK (PipeWire)"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getLastError() const override { return lastError_; }
    bool isAvailable() const override { return true; }

private:
    static int processCallback(jack_nframes_t nframes, void* arg);
    static void shutdownCallback(void* arg);

    jack_client_t* client_ = nullptr;
    jack_port_t* inputPort_ = nullptr;
    jack_port_t* outputPorts_[2] = {nullptr, nullptr};

    AudioCallback callback_;
    AudioConfig config_;
    bool initialized_ = false;
    std::atomic<bool> running_{false};
    std::string lastError_;

    mutable std::mutex devicesMutex_;
    std::vector<AudioDevice> inputDevices_;
    std::vector<AudioDevice> outputDevices_;

    // Metering
    std::atomic<float> inputLevel_{ -60.0f };
    std::atomic<float> outputLevelLeft_{ -60.0f };
    std::atomic<float> outputLevelRight_{ -60.0f };
    
    float linearToDb(float linear);
};

} // namespace openamp
