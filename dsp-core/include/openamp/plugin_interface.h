#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace openamp {

struct AudioBuffer {
    float* data;
    uint32_t numChannels;
    uint32_t numFrames;
    uint32_t sampleRate;
};

class AudioProcessor {
public:
    virtual ~AudioProcessor() = default;
    
    virtual void prepare(double sampleRate, uint32_t maxBlockSize) = 0;
    virtual void process(AudioBuffer& buffer) = 0;
    virtual void reset() = 0;
    
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
};

using AudioProcessorPtr = std::unique_ptr<AudioProcessor>;

class PluginInterface {
public:
    virtual ~PluginInterface() = default;
    virtual AudioProcessorPtr createProcessor() = 0;
    virtual std::string getPluginName() const = 0;
    virtual std::string getPluginVersion() const = 0;
};

using CreatePluginFunc = PluginInterface* (*)();
using DestroyPluginFunc = void (*)(PluginInterface*);

#define GUITAR_AMP_PLUGIN_API extern "C"

} // namespace openamp