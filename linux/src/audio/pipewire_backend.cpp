#ifdef USE_PIPEWIRE

#include "pipewire_backend.h"
#include <spa/utils/result.h>
#include <spa/utils/json.h>
#include <cstring>
#include <algorithm>
#include <chrono>

namespace openamp {

PipeWireBackend::PipeWireBackend() = default;

PipeWireBackend::~PipeWireBackend() {
    shutdown();
}

bool PipeWireBackend::initialize() {
    pw_init(nullptr, nullptr);

    mainLoop_ = pw_main_loop_new(nullptr);
    if (!mainLoop_) {
        lastError_ = "Failed to create PipeWire main loop";
        return false;
    }

    context_ = pw_context_new(pw_main_loop_get_loop(mainLoop_), nullptr, 0);
    if (!context_) {
        lastError_ = "Failed to create PipeWire context";
        shutdown();
        return false;
    }

    core_ = pw_context_connect(context_, nullptr, 0);
    if (!core_) {
        lastError_ = "Failed to connect to PipeWire daemon";
        shutdown();
        return false;
    }

    registry_ = pw_core_get_registry(core_, PW_VERSION_REGISTRY, 0);
    if (!registry_) {
        lastError_ = "Failed to get PipeWire registry";
        shutdown();
        return false;
    }

    // Set up registry listener using member hook
    static const pw_registry_events registryEvents = {
        PW_VERSION_REGISTRY_EVENTS,
        PipeWireBackend::onRegistryGlobal,
        PipeWireBackend::onRegistryGlobalRemove,
    };
    
    spa_zero(registryListener_);
    pw_registry_add_listener(registry_, &registryListener_, &registryEvents, this);

    // Start the main loop in a separate thread to receive events
    loopThread_ = std::thread([this]() {
        pw_main_loop_run(mainLoop_);
    });

    // Wait a bit for devices to be enumerated
    {
        std::unique_lock<std::mutex> lock(devicesMutex_);
        initCv_.wait_for(lock, std::chrono::milliseconds(500), [this] { return devicesReady_; });
    }

    // Always add default options
    {
        std::lock_guard<std::mutex> lock(devicesMutex_);
        
        AudioDevice defaultInput;
        defaultInput.id = "default";
        defaultInput.name = "Default Input";
        defaultInput.isInput = true;
        defaultInput.isDefault = true;
        defaultInput.numChannels = 1;
        defaultInput.sampleRate = 48000;
        inputDevices_.insert(inputDevices_.begin(), {PW_ID_ANY, defaultInput});
        
        AudioDevice defaultOutput;
        defaultOutput.id = "default";
        defaultOutput.name = "Default Output";
        defaultOutput.isInput = false;
        defaultOutput.isDefault = true;
        defaultOutput.numChannels = 2;
        defaultOutput.sampleRate = 48000;
        outputDevices_.insert(outputDevices_.begin(), {PW_ID_ANY, defaultOutput});
    }

    initialized_ = true;
    return true;
}

void PipeWireBackend::shutdown() {
    if (running_) {
        stop();
    }

    if (mainLoop_) {
        pw_main_loop_quit(mainLoop_);
    }

    if (loopThread_.joinable()) {
        loopThread_.join();
    }

    if (registry_) {
        pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry_));
        registry_ = nullptr;
    }

    if (core_) {
        pw_core_disconnect(core_);
        core_ = nullptr;
    }

    if (context_) {
        pw_context_destroy(context_);
        context_ = nullptr;
    }

    if (mainLoop_) {
        pw_main_loop_destroy(mainLoop_);
        mainLoop_ = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(devicesMutex_);
        inputDevices_.clear();
        outputDevices_.clear();
    }

    initialized_ = false;
}

bool PipeWireBackend::isInitialized() const {
    return initialized_;
}

void PipeWireBackend::onRegistryGlobal(void* data, uint32_t id, uint32_t permissions,
                                        const char* type, uint32_t version,
                                        const struct spa_dict* props) {
    auto* backend = static_cast<PipeWireBackend*>(data);
    
    if (!type || !props) return;
    
    // Only interested in nodes (audio devices)
    if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0) return;
    
    const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!mediaClass) return;
    
    // Only interested in Audio/Source (input) and Audio/Sink (output)
    bool isInput = (strcmp(mediaClass, "Audio/Source") == 0 || 
                    strcmp(mediaClass, "Audio/Source/Virtual") == 0);
    bool isOutput = (strcmp(mediaClass, "Audio/Sink") == 0);
    
    if (!isInput && !isOutput) return;
    
    const char* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    const char* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    const char* nick = spa_dict_lookup(props, PW_KEY_NODE_NICK);
    
    if (!name) return;
    
    AudioDevice device;
    device.id = std::to_string(id);
    device.name = desc ? desc : (nick ? nick : name);
    device.isInput = isInput;
    device.isDefault = false;
    
    // Get channel count
    const char* channels = spa_dict_lookup(props, PW_KEY_AUDIO_CHANNELS);
    device.numChannels = channels ? atoi(channels) : (isInput ? 1 : 2);
    
    // Get sample rate
    const char* rate = spa_dict_lookup(props, "node.rate");
    device.sampleRate = rate ? atoi(rate) : 48000;
    
    std::lock_guard<std::mutex> lock(backend->devicesMutex_);
    
    DeviceInfo info{id, device};
    
    if (isInput) {
        // Remove existing entry with same id
        backend->inputDevices_.erase(
            std::remove_if(backend->inputDevices_.begin(), backend->inputDevices_.end(),
                          [id](const DeviceInfo& d) { return d.id == id; }),
            backend->inputDevices_.end());
        backend->inputDevices_.push_back(info);
    } else {
        backend->outputDevices_.erase(
            std::remove_if(backend->outputDevices_.begin(), backend->outputDevices_.end(),
                          [id](const DeviceInfo& d) { return d.id == id; }),
            backend->outputDevices_.end());
        backend->outputDevices_.push_back(info);
    }
    
    // Signal that we have devices
    if (!backend->inputDevices_.empty() && !backend->outputDevices_.empty()) {
        backend->devicesReady_ = true;
        backend->initCv_.notify_one();
    }
}

void PipeWireBackend::onRegistryGlobalRemove(void* data, uint32_t id) {
    auto* backend = static_cast<PipeWireBackend*>(data);
    std::lock_guard<std::mutex> lock(backend->devicesMutex_);
    
    backend->inputDevices_.erase(
        std::remove_if(backend->inputDevices_.begin(), backend->inputDevices_.end(),
                      [id](const DeviceInfo& d) { return d.id == id; }),
        backend->inputDevices_.end());
    
    backend->outputDevices_.erase(
        std::remove_if(backend->outputDevices_.begin(), backend->outputDevices_.end(),
                      [id](const DeviceInfo& d) { return d.id == id; }),
        backend->outputDevices_.end());
}

int PipeWireBackend::onMetadataProperty(void* data, uint32_t subject, const char* key,
                                          const char* type, const char* value) {
    auto* backend = static_cast<PipeWireBackend*>(data);
    if (!key || !value) return 0;
    
    std::lock_guard<std::mutex> lock(backend->devicesMutex_);
    
    if (strcmp(key, "default.audio.source") == 0) {
        backend->defaultInputName_ = value;
        // Update isDefault flag
        for (auto& info : backend->inputDevices_) {
            info.device.isDefault = (info.device.name == value);
        }
    } else if (strcmp(key, "default.audio.sink") == 0) {
        backend->defaultOutputName_ = value;
        for (auto& info : backend->outputDevices_) {
            info.device.isDefault = (info.device.name == value);
        }
    }
    return 0;
}

std::vector<AudioDevice> PipeWireBackend::getInputDevices() {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    std::vector<AudioDevice> result;
    result.push_back({"default", "Default Input", true, true, 1, 48000});
    for (const auto& info : inputDevices_) {
        result.push_back(info.device);
    }
    return result;
}

std::vector<AudioDevice> PipeWireBackend::getOutputDevices() {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    std::vector<AudioDevice> result;
    result.push_back({"default", "Default Output", false, true, 2, 48000});
    for (const auto& info : outputDevices_) {
        result.push_back(info.device);
    }
    return result;
}

AudioDevice PipeWireBackend::getDefaultInputDevice() {
    auto devices = getInputDevices();
    for (const auto& d : devices) {
        if (d.isDefault) return d;
    }
    return devices.empty() ? AudioDevice{} : devices[0];
}

AudioDevice PipeWireBackend::getDefaultOutputDevice() {
    auto devices = getOutputDevices();
    for (const auto& d : devices) {
        if (d.isDefault) return d;
    }
    return devices.empty() ? AudioDevice{} : devices[0];
}

bool PipeWireBackend::setConfig(const AudioConfig& config) {
    if (running_) {
        lastError_ = "Cannot change config while running";
        return false;
    }
    config_ = config;
    
    // Parse device IDs
    if (config.inputDeviceId != "default" && !config.inputDeviceId.empty()) {
        inputNodeId_ = std::stoul(config.inputDeviceId);
    } else {
        inputNodeId_ = PW_ID_ANY;
    }
    
    if (config.outputDeviceId != "default" && !config.outputDeviceId.empty()) {
        outputNodeId_ = std::stoul(config.outputDeviceId);
    } else {
        outputNodeId_ = PW_ID_ANY;
    }
    
    return true;
}

AudioConfig PipeWireBackend::getConfig() const {
    return config_;
}

bool PipeWireBackend::start(AudioCallback callback) {
    if (!initialized_) {
        lastError_ = "Backend not initialized";
        return false;
    }

    if (running_) {
        stop();
    }

    callback_ = callback;

    // Setup input stream
    spa_audio_info_raw inputInfo = {};
    inputInfo.format = SPA_AUDIO_FORMAT_F32;
    inputInfo.channels = config_.inputChannels;
    inputInfo.rate = config_.sampleRate;
    if (config_.inputChannels == 1) {
        inputInfo.position[0] = SPA_AUDIO_CHANNEL_MONO;
    } else {
        inputInfo.position[0] = SPA_AUDIO_CHANNEL_FL;
        inputInfo.position[1] = SPA_AUDIO_CHANNEL_FR;
    }

    uint8_t inputBuffer[1024];
    spa_pod_builder inputBuilder = SPA_POD_BUILDER_INIT(inputBuffer, sizeof(inputBuffer));
    const spa_pod* inputParams[1];
    inputParams[0] = spa_format_audio_raw_build(&inputBuilder, SPA_PARAM_EnumFormat, &inputInfo);

    pw_properties* inputProps = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Music",
        PW_KEY_NODE_NAME, "OpenAmp Input",
        nullptr
    );

    inputStream_ = pw_stream_new(core_, "OpenAmp Input", inputProps);
    if (!inputStream_) {
        lastError_ = "Failed to create input stream";
        return false;
    }

    static const pw_stream_events inputEvents = {
        PW_VERSION_STREAM_EVENTS,
        PipeWireBackend::onInputProcess,
    };
    
    spa_zero(inputStreamListener_);
    pw_stream_add_listener(inputStream_, &inputStreamListener_, &inputEvents, this);

    if (pw_stream_connect(inputStream_,
                          PW_DIRECTION_INPUT,
                          inputNodeId_,
                          static_cast<pw_stream_flags>(
                              PW_STREAM_FLAG_AUTOCONNECT |
                              PW_STREAM_FLAG_MAP_BUFFERS |
                              PW_STREAM_FLAG_RT_PROCESS),
                          inputParams, 1) < 0) {
        lastError_ = "Failed to connect input stream";
        return false;
    }

    // Setup output stream
    spa_audio_info_raw outputInfo = {};
    outputInfo.format = SPA_AUDIO_FORMAT_F32;
    outputInfo.channels = config_.outputChannels;
    outputInfo.rate = config_.sampleRate;
    outputInfo.position[0] = SPA_AUDIO_CHANNEL_FL;
    outputInfo.position[1] = SPA_AUDIO_CHANNEL_FR;

    uint8_t outputBuffer[1024];
    spa_pod_builder outputBuilder = SPA_POD_BUILDER_INIT(outputBuffer, sizeof(outputBuffer));
    const spa_pod* outputParams[1];
    outputParams[0] = spa_format_audio_raw_build(&outputBuilder, SPA_PARAM_EnumFormat, &outputInfo);

    pw_properties* outputProps = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Music",
        PW_KEY_NODE_NAME, "OpenAmp Output",
        nullptr
    );

    outputStream_ = pw_stream_new(core_, "OpenAmp Output", outputProps);
    if (!outputStream_) {
        lastError_ = "Failed to create output stream";
        pw_stream_destroy(inputStream_);
        inputStream_ = nullptr;
        return false;
    }

    static const pw_stream_events outputEvents = {
        PW_VERSION_STREAM_EVENTS,
        PipeWireBackend::onOutputProcess,
    };
    
    spa_zero(outputStreamListener_);
    pw_stream_add_listener(outputStream_, &outputStreamListener_, &outputEvents, this);

    if (pw_stream_connect(outputStream_,
                          PW_DIRECTION_OUTPUT,
                          outputNodeId_,
                          static_cast<pw_stream_flags>(
                              PW_STREAM_FLAG_AUTOCONNECT |
                              PW_STREAM_FLAG_MAP_BUFFERS |
                              PW_STREAM_FLAG_RT_PROCESS),
                          outputParams, 1) < 0) {
        lastError_ = "Failed to connect output stream";
        pw_stream_destroy(inputStream_);
        inputStream_ = nullptr;
        return false;
    }

    running_ = true;
    return true;
}

void PipeWireBackend::stop() {
    if (!running_) return;

    running_ = false;

    if (inputStream_) {
        pw_stream_disconnect(inputStream_);
        pw_stream_destroy(inputStream_);
        inputStream_ = nullptr;
    }

    if (outputStream_) {
        pw_stream_disconnect(outputStream_);
        pw_stream_destroy(outputStream_);
        outputStream_ = nullptr;
    }
}

bool PipeWireBackend::isRunning() const {
    return running_;
}

double PipeWireBackend::getInputLatency() const {
    return static_cast<double>(config_.bufferSize) / config_.sampleRate * 1000.0;
}

double PipeWireBackend::getOutputLatency() const {
    return static_cast<double>(config_.bufferSize) / config_.sampleRate * 1000.0;
}

std::string PipeWireBackend::getVersion() const {
    return pw_get_library_version();
}

void PipeWireBackend::onInputProcess(void* userdata) {
    auto* backend = static_cast<PipeWireBackend*>(userdata);
    backend->handleInputProcess();
}

void PipeWireBackend::onOutputProcess(void* userdata) {
    auto* backend = static_cast<PipeWireBackend*>(userdata);
    backend->handleOutputProcess();
}

void PipeWireBackend::handleInputProcess() {
    // Input is handled in handleOutputProcess
}

void PipeWireBackend::handleOutputProcess() {
    if (!callback_ || !running_) return;

    // Get input buffer
    pw_buffer* inBuf = inputStream_ ? pw_stream_dequeue_buffer(inputStream_) : nullptr;
    
    // Get output buffer
    pw_buffer* outBuf = pw_stream_dequeue_buffer(outputStream_);
    if (!outBuf) {
        if (inBuf) pw_stream_queue_buffer(inputStream_, inBuf);
        return;
    }

    // Get data pointers
    spa_data* outData = &outBuf->buffer->datas[0];
    float* outputData = static_cast<float*>(outData->data);
    
    if (!outputData) {
        if (inBuf) pw_stream_queue_buffer(inputStream_, inBuf);
        pw_stream_queue_buffer(outputStream_, outBuf);
        return;
    }

    // Calculate frames
    uint32_t frames = config_.bufferSize;
    
    // Create a temporary mono input buffer
    std::vector<float> tempInput(frames, 0.0f);
    
    if (inBuf) {
        spa_data* inData = &inBuf->buffer->datas[0];
        float* inputData = static_cast<float*>(inData->data);
        if (inputData) {
            uint32_t inFrames = inData->chunk ? (inData->chunk->size / sizeof(float)) : frames;
            uint32_t copyFrames = std::min(inFrames, frames);
            
            // Convert to mono if needed (take left channel or average)
            if (inBuf->buffer->n_datas > 1) {
                for (uint32_t i = 0; i < copyFrames; ++i) {
                    spa_data* leftData = &inBuf->buffer->datas[0];
                    spa_data* rightData = &inBuf->buffer->datas[1];
                    float* left = static_cast<float*>(leftData->data);
                    float* right = static_cast<float*>(rightData->data);
                    if (left && right) {
                        tempInput[i] = (left[i] + right[i]) * 0.5f;
                    } else if (left) {
                        tempInput[i] = left[i];
                    }
                }
            } else {
                memcpy(tempInput.data(), inputData, copyFrames * sizeof(float));
            }
        }
        pw_stream_queue_buffer(inputStream_, inBuf);
    }

    // Process audio (input is mono, output is stereo interleaved)
    callback_(tempInput.data(), outputData, frames);

    // Update meters
    updateMeters(tempInput.data(), outputData, frames);

    // Set output buffer size
    outBuf->size = frames;
    pw_stream_queue_buffer(outputStream_, outBuf);
}

float PipeWireBackend::linearToDb(float linear) {
    if (linear <= 0.0f) return -60.0f;
    return 20.0f * std::log10(linear);
}

void PipeWireBackend::updateMeters(const float* input, const float* output, uint32_t numFrames) {
    // Calculate input RMS (mono)
    float inputSumSq = 0.0f;
    float inputPeak = 0.0f;
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = std::abs(input[i]);
        inputSumSq += sample * sample;
        if (sample > inputPeak) inputPeak = sample;
    }
    float inputRms = std::sqrt(inputSumSq / numFrames);
    float inputDb = linearToDb(inputRms);
    float inputPeakDb = linearToDb(inputPeak);
    
    // Calculate output RMS (stereo interleaved)
    float leftSumSq = 0.0f, rightSumSq = 0.0f;
    float leftPeak = 0.0f, rightPeak = 0.0f;
    for (uint32_t i = 0; i < numFrames; ++i) {
        float left = std::abs(output[i * 2]);
        float right = std::abs(output[i * 2 + 1]);
        leftSumSq += left * left;
        rightSumSq += right * right;
        if (left > leftPeak) leftPeak = left;
        if (right > rightPeak) rightPeak = right;
    }
    float leftRms = std::sqrt(leftSumSq / numFrames);
    float rightRms = std::sqrt(rightSumSq / numFrames);
    
    // Store levels
    inputLevelLeft_.store(inputDb);
    inputLevelRight_.store(inputDb);  // Mono input, same for both
    
    outputLevelLeft_.store(linearToDb(leftRms));
    outputLevelRight_.store(linearToDb(rightRms));
    
    // Update peak hold with decay
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - lastPeakUpdate_).count();
    
    if (elapsed > 0.01f) {  // Update peaks every 10ms
        lastPeakUpdate_ = now;
        float peakDecay = elapsed * PEAK_DECAY_DB_PER_SEC;
        
        // Input peaks
        float currentInputPeak = inputPeakLeft_.load();
        if (inputPeakDb > currentInputPeak) {
            inputPeakLeft_.store(inputPeakDb);
            inputPeakRight_.store(inputPeakDb);
        } else {
            inputPeakLeft_.store(std::max(-60.0f, currentInputPeak - peakDecay));
            inputPeakRight_.store(std::max(-60.0f, inputPeakRight_.load() - peakDecay));
        }
        
        // Output peaks
        float leftPeakDb = linearToDb(leftPeak);
        float rightPeakDb = linearToDb(rightPeak);
        
        float currentLeftPeak = outputPeakLeft_.load();
        float currentRightPeak = outputPeakRight_.load();
        
        if (leftPeakDb > currentLeftPeak) {
            outputPeakLeft_.store(leftPeakDb);
        } else {
            outputPeakLeft_.store(std::max(-60.0f, currentLeftPeak - peakDecay));
        }
        
        if (rightPeakDb > currentRightPeak) {
            outputPeakRight_.store(rightPeakDb);
        } else {
            outputPeakRight_.store(std::max(-60.0f, currentRightPeak - peakDecay));
        }
    }
}

MeterLevels PipeWireBackend::getInputLevels() const {
    MeterLevels levels;
    levels.leftDb = inputLevelLeft_.load();
    levels.rightDb = inputLevelRight_.load();
    levels.leftPeakDb = inputPeakLeft_.load();
    levels.rightPeakDb = inputPeakRight_.load();
    return levels;
}

MeterLevels PipeWireBackend::getOutputLevels() const {
    MeterLevels levels;
    levels.leftDb = outputLevelLeft_.load();
    levels.rightDb = outputLevelRight_.load();
    levels.leftPeakDb = outputPeakLeft_.load();
    levels.rightPeakDb = outputPeakRight_.load();
    return levels;
}

void PipeWireBackend::resetPeakHold() {
    inputPeakLeft_.store(-60.0f);
    inputPeakRight_.store(-60.0f);
    outputPeakLeft_.store(-60.0f);
    outputPeakRight_.store(-60.0f);
}

} // namespace openamp

#endif // USE_PIPEWIRE
