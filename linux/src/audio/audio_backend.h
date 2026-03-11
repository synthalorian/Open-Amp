#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

namespace openamp {

struct AudioDevice {
    std::string id;
    std::string name;
    bool isInput;
    bool isDefault;
    int numChannels;
    int sampleRate;
};

struct AudioConfig {
    int sampleRate = 48000;
    int bufferSize = 256;
    int inputChannels = 1;
    int outputChannels = 2;
    std::string inputDeviceId;
    std::string outputDeviceId;
};

struct MeterLevels {
    float leftDb = -60.0f;      // Left channel level in dB
    float rightDb = -60.0f;     // Right channel level in dB
    float leftPeakDb = -60.0f;  // Left peak hold in dB
    float rightPeakDb = -60.0f; // Right peak hold in dB
};

using AudioCallback = std::function<void(float* input, float* output, uint32_t numFrames)>;

class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    // Lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    // Device enumeration
    virtual std::vector<AudioDevice> getInputDevices() = 0;
    virtual std::vector<AudioDevice> getOutputDevices() = 0;
    virtual AudioDevice getDefaultInputDevice() = 0;
    virtual AudioDevice getDefaultOutputDevice() = 0;

    // Configuration
    virtual bool setConfig(const AudioConfig& config) = 0;
    virtual AudioConfig getConfig() const = 0;

    // Audio stream
    virtual bool start(AudioCallback callback) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    // Latency info
    virtual double getInputLatency() const = 0;
    virtual double getOutputLatency() const = 0;

    // Metering
    virtual MeterLevels getInputLevels() const = 0;
    virtual MeterLevels getOutputLevels() const = 0;
    virtual void resetPeakHold() = 0;

    // Backend info
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual bool isAvailable() const = 0;

    // Error handling
    virtual std::string getLastError() const = 0;
};

// Factory function
std::unique_ptr<AudioBackend> createBestBackend();
std::unique_ptr<AudioBackend> createPipeWireBackend();
std::unique_ptr<AudioBackend> createALSABackend();

} // namespace openamp
