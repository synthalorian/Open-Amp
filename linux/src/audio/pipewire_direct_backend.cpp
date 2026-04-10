#include "pipewire_direct_backend.h"
#include <spa/param/audio/format-utils.h>
#include <spa/debug/types.h>
#include <cmath>
#include <iostream>

namespace openamp {

PipeWireDirectBackend::PipeWireDirectBackend() = default;

PipeWireDirectBackend::~PipeWireDirectBackend() {
    shutdown();
}

bool PipeWireDirectBackend::initialize() {
    pw_init(nullptr, nullptr);
    mainLoop_ = pw_main_loop_new(nullptr);
    context_ = pw_context_new(pw_main_loop_get_loop(mainLoop_), nullptr, 0);
    core_ = pw_context_connect(context_, nullptr, 0);
    
    if (!core_) {
        lastError_ = "Failed to connect to PipeWire";
        return false;
    }

    registry_ = pw_core_get_registry(core_, PW_VERSION_REGISTRY, 0);
    
    static const pw_registry_events registry_events = {
        PW_VERSION_REGISTRY_EVENTS,
        .global = onRegistryGlobal,
    };
    
    struct spa_hook listener;
    pw_registry_add_listener(registry_, &listener, &registry_events, this);

    loopThread_ = std::thread([this]() {
        pw_main_loop_run(mainLoop_);
    });

    // Wait for registry sync
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    initialized_ = true;
    return true;
}

void PipeWireDirectBackend::onRegistryGlobal(void* data, uint32_t id, uint32_t permissions,
                                             const char* type, uint32_t version,
                                             const struct spa_dict* props) {
    auto* self = static_cast<PipeWireDirectBackend*>(data);
    if (std::string(type) == PW_TYPE_INTERFACE_Node) {
        const char* name = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
        const char* nick = spa_dict_lookup(props, PW_KEY_NODE_NICK);
        const char* path = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        const char* media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);

        if (!media_class) return;

        AudioDevice dev;
        dev.id = path ? path : std::to_string(id);
        dev.name = name ? name : (nick ? nick : dev.id);
        
        std::lock_guard<std::mutex> lock(self->devicesMutex_);
        if (std::string(media_class).find("Audio/Source") != std::string::npos) {
            dev.isInput = true;
            self->inputDevices_.push_back(dev);
        } else if (std::string(media_class).find("Audio/Sink") != std::string::npos) {
            dev.isInput = false;
            self->outputDevices_.push_back(dev);
        }
    }
}

bool PipeWireDirectBackend::start(AudioCallback callback) {
    callback_ = callback;
    
    const struct pw_stream_events input_stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .process = onStreamProcess,
    };

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_pod* params[1];
    
    struct spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT();
    info.format = SPA_AUDIO_FORMAT_F32;
    info.rate = config_.sampleRate;
    info.channels = 1;
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

    inputStream_ = pw_stream_new(core_, "OpenAmp Input",
                                 pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                                                   PW_KEY_MEDIA_CATEGORY, "Capture",
                                                   PW_KEY_NODE_AUTOCONNECT, "true",
                                                   nullptr));
    
    pw_stream_add_listener(inputStream_, &listener_, &input_stream_events, this);
    
    pw_stream_connect(inputStream_, PW_DIRECTION_INPUT, 
                      config_.inputDeviceId.empty() ? PW_ID_ANY : std::stoi(config_.inputDeviceId),
                      (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
                      params, 1);

    running_ = true;
    return true;
}

void PipeWireDirectBackend::onStreamProcess(void* data) {
    auto* self = static_cast<PipeWireDirectBackend*>(data);
    struct pw_buffer* b;
    if (!(b = pw_stream_dequeue_buffer(self->inputStream_))) return;

    float* in = (float*)b->buffer->datas[0].data;
    uint32_t n_frames = b->buffer->datas[0].chunk->size / sizeof(float);

    // Calculate Input Level
    float sum = 0;
    for(uint32_t i=0; i<n_frames; ++i) sum += std::abs(in[i]);
    self->inputLevel_.store(self->linearToDb(sum/n_frames));

    // Process
    static float outBuf[8192*2];
    self->callback_(in, outBuf, n_frames);

    pw_stream_queue_buffer(self->inputStream_, b);
}

// ... rest of boilerplate stubs for header compliance ...
void PipeWireDirectBackend::shutdown() { if(mainLoop_) pw_main_loop_quit(mainLoop_); }
bool PipeWireDirectBackend::isInitialized() const { return initialized_; }
std::vector<AudioDevice> PipeWireDirectBackend::getInputDevices() { return inputDevices_; }
std::vector<AudioDevice> PipeWireDirectBackend::getOutputDevices() { return outputDevices_; }
AudioDevice PipeWireDirectBackend::getDefaultInputDevice() { return inputDevices_.empty() ? AudioDevice{} : inputDevices_[0]; }
AudioDevice PipeWireDirectBackend::getDefaultOutputDevice() { return outputDevices_.empty() ? AudioDevice{} : outputDevices_[0]; }
bool PipeWireDirectBackend::setConfig(const AudioConfig& config) { config_ = config; return true; }
AudioConfig PipeWireDirectBackend::getConfig() const { return config_; }
void PipeWireDirectBackend::stop() { running_ = false; }
bool PipeWireDirectBackend::isRunning() const { return running_; }
double PipeWireDirectBackend::getInputLatency() const { return 0; }
double PipeWireDirectBackend::getOutputLatency() const { return 0; }
MeterLevels PipeWireDirectBackend::getInputLevels() const { return {inputLevel_.load()}; }
MeterLevels PipeWireDirectBackend::getOutputLevels() const { return {outputLevelLeft_.load(), outputLevelRight_.load()}; }
void PipeWireDirectBackend::resetPeakHold() {}
float PipeWireDirectBackend::linearToDb(float linear) { return linear <= 0.00001f ? -60.0f : 20.0f * std::log10(linear); }
std::string PipeWireDirectBackend::getVersion() const { return "1.0.0"; }

} // namespace openamp
