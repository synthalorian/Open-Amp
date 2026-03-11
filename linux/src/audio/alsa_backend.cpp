#ifdef USE_ALSA

#include "alsa_backend.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace openamp {

ALSABackend::ALSABackend() = default;

ALSABackend::~ALSABackend() {
    shutdown();
}

bool ALSABackend::initialize() {
    scanDevices();
    initialized_ = true;
    return true;
}

void ALSABackend::shutdown() {
    if (running_) {
        stop();
    }
    initialized_ = false;
}

bool ALSABackend::isInitialized() const {
    return initialized_;
}

std::vector<AudioDevice> ALSABackend::getInputDevices() {
    return inputDevices_;
}

std::vector<AudioDevice> ALSABackend::getOutputDevices() {
    return outputDevices_;
}

AudioDevice ALSABackend::getDefaultInputDevice() {
    auto devices = getInputDevices();
    for (const auto& d : devices) {
        if (d.isDefault) return d;
    }
    AudioDevice dev;
    dev.id = "default";
    dev.name = "Default Input";
    dev.isInput = true;
    dev.isDefault = true;
    return dev;
}

AudioDevice ALSABackend::getDefaultOutputDevice() {
    auto devices = getOutputDevices();
    for (const auto& d : devices) {
        if (d.isDefault) return d;
    }
    AudioDevice dev;
    dev.id = "default";
    dev.name = "Default Output";
    dev.isInput = false;
    dev.isDefault = true;
    return dev;
}

bool ALSABackend::setConfig(const AudioConfig& config) {
    if (running_) {
        lastError_ = "Cannot change config while running";
        return false;
    }
    config_ = config;
    return true;
}

AudioConfig ALSABackend::getConfig() const {
    return config_;
}

bool ALSABackend::setupPCM(snd_pcm_t** pcm, const std::string& device, bool isInput) {
    snd_pcm_stream_t stream = isInput ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;

    int err = snd_pcm_open(pcm, device.c_str(), stream, 0);
    if (err < 0) {
        lastError_ = "Failed to open PCM device: " + std::string(snd_strerror(err));
        return false;
    }

    snd_pcm_hw_params_t* hwParams;
    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_hw_params_any(*pcm, hwParams);

    // Set access type
    err = snd_pcm_hw_params_set_access(*pcm, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        lastError_ = "Failed to set access type: " + std::string(snd_strerror(err));
        snd_pcm_close(*pcm);
        *pcm = nullptr;
        return false;
    }

    // Set format (32-bit float)
    err = snd_pcm_hw_params_set_format(*pcm, hwParams, SND_PCM_FORMAT_FLOAT_LE);
    if (err < 0) {
        // Try 16-bit as fallback
        err = snd_pcm_hw_params_set_format(*pcm, hwParams, SND_PCM_FORMAT_S16_LE);
        if (err < 0) {
            lastError_ = "Failed to set format: " + std::string(snd_strerror(err));
            snd_pcm_close(*pcm);
            *pcm = nullptr;
            return false;
        }
    }

    // Set channels
    int channels = isInput ? config_.inputChannels : config_.outputChannels;
    err = snd_pcm_hw_params_set_channels(*pcm, hwParams, channels);
    if (err < 0) {
        lastError_ = "Failed to set channels: " + std::string(snd_strerror(err));
        snd_pcm_close(*pcm);
        *pcm = nullptr;
        return false;
    }

    // Set sample rate
    unsigned int rate = config_.sampleRate;
    err = snd_pcm_hw_params_set_rate_near(*pcm, hwParams, &rate, nullptr);
    if (err < 0) {
        lastError_ = "Failed to set sample rate: " + std::string(snd_strerror(err));
        snd_pcm_close(*pcm);
        *pcm = nullptr;
        return false;
    }

    // Set buffer size
    snd_pcm_uframes_t bufferSize = config_.bufferSize;
    err = snd_pcm_hw_params_set_buffer_size_near(*pcm, hwParams, &bufferSize);
    if (err < 0) {
        lastError_ = "Failed to set buffer size: " + std::string(snd_strerror(err));
        snd_pcm_close(*pcm);
        *pcm = nullptr;
        return false;
    }

    // Set period size
    snd_pcm_uframes_t periodSize = bufferSize / 2;
    err = snd_pcm_hw_params_set_period_size_near(*pcm, hwParams, &periodSize, nullptr);
    if (err < 0) {
        lastError_ = "Failed to set period size: " + std::string(snd_strerror(err));
        snd_pcm_close(*pcm);
        *pcm = nullptr;
        return false;
    }

    // Apply parameters
    err = snd_pcm_hw_params(*pcm, hwParams);
    if (err < 0) {
        lastError_ = "Failed to apply HW params: " + std::string(snd_strerror(err));
        snd_pcm_close(*pcm);
        *pcm = nullptr;
        return false;
    }

    // Calculate latency
    snd_pcm_uframes_t actualBufferSize;
    snd_pcm_hw_params_get_buffer_size(hwParams, &actualBufferSize);
    double latency = static_cast<double>(actualBufferSize) / config_.sampleRate * 1000.0;

    if (isInput) {
        inputLatency_ = latency;
    } else {
        outputLatency_ = latency;
    }

    // Setup software parameters
    snd_pcm_sw_params_t* swParams;
    snd_pcm_sw_params_alloca(&swParams);
    snd_pcm_sw_params_current(*pcm, swParams);

    err = snd_pcm_sw_params_set_avail_min(*pcm, swParams, periodSize);
    if (err < 0) {
        lastError_ = "Failed to set avail_min: " + std::string(snd_strerror(err));
        snd_pcm_close(*pcm);
        *pcm = nullptr;
        return false;
    }

    err = snd_pcm_sw_params(*pcm, swParams);
    if (err < 0) {
        lastError_ = "Failed to apply SW params: " + std::string(snd_strerror(err));
        snd_pcm_close(*pcm);
        *pcm = nullptr;
        return false;
    }

    return true;
}

bool ALSABackend::start(AudioCallback callback) {
    if (!initialized_) {
        lastError_ = "Backend not initialized";
        return false;
    }

    if (running_) {
        stop();
    }

    callback_ = callback;

    // Setup input PCM
    std::string inputDevice = config_.inputDeviceId.empty() ? "default" : config_.inputDeviceId;
    if (!setupPCM(&inputPCM_, inputDevice, true)) {
        return false;
    }

    // Setup output PCM
    std::string outputDevice = config_.outputDeviceId.empty() ? "default" : config_.outputDeviceId;
    if (!setupPCM(&outputPCM_, outputDevice, false)) {
        snd_pcm_close(inputPCM_);
        inputPCM_ = nullptr;
        return false;
    }

    // Prepare PCM devices
    snd_pcm_prepare(inputPCM_);
    snd_pcm_prepare(outputPCM_);

    // Start capture
    snd_pcm_start(inputPCM_);

    running_ = true;
    audioThread_ = std::thread(&ALSABackend::audioThread, this);

    return true;
}

void ALSABackend::stop() {
    if (!running_) return;

    running_ = false;

    if (audioThread_.joinable()) {
        audioThread_.join();
    }

    if (inputPCM_) {
        snd_pcm_drop(inputPCM_);
        snd_pcm_close(inputPCM_);
        inputPCM_ = nullptr;
    }

    if (outputPCM_) {
        snd_pcm_drop(outputPCM_);
        snd_pcm_close(outputPCM_);
        outputPCM_ = nullptr;
    }
}

bool ALSABackend::isRunning() const {
    return running_;
}

double ALSABackend::getInputLatency() const {
    return inputLatency_;
}

double ALSABackend::getOutputLatency() const {
    return outputLatency_;
}

std::string ALSABackend::getVersion() const {
    return snd_asoundlib_version();
}

void ALSABackend::scanDevices() {
    inputDevices_.clear();
    outputDevices_.clear();

    // Scan for PCM devices
    void** hints = nullptr;
    if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
        return;
    }

    for (void** hint = hints; *hint != nullptr; ++hint) {
        char* name = snd_device_name_get_hint(*hint, "NAME");
        char* desc = snd_device_name_get_hint(*hint, "DESC");
        char* io = snd_device_name_get_hint(*hint, "IOID");

        if (name && strcmp(name, "null") != 0) {
            bool isInput = (io == nullptr || strcmp(io, "Input") == 0);
            bool isOutput = (io == nullptr || strcmp(io, "Output") == 0);

            if (isInput) {
                AudioDevice dev;
                dev.id = name;
                dev.name = desc ? desc : name;
                dev.isInput = true;
                dev.isDefault = (strcmp(name, "default") == 0);
                dev.numChannels = 1;
                dev.sampleRate = 48000;
                inputDevices_.push_back(dev);
            }

            if (isOutput) {
                AudioDevice dev;
                dev.id = name;
                dev.name = desc ? desc : name;
                dev.isInput = false;
                dev.isDefault = (strcmp(name, "default") == 0);
                dev.numChannels = 2;
                dev.sampleRate = 48000;
                outputDevices_.push_back(dev);
            }
        }

        if (name) free(name);
        if (desc) free(desc);
        if (io) free(io);
    }

    snd_device_name_free_hint(hints);
}

void ALSABackend::audioThread() {
    std::vector<float> inputBuffer(config_.bufferSize * config_.inputChannels);
    std::vector<float> outputBuffer(config_.bufferSize * config_.outputChannels);

    while (running_) {
        // Read input
        snd_pcm_sframes_t framesRead = snd_pcm_readi(inputPCM_, inputBuffer.data(), config_.bufferSize);

        if (framesRead < 0) {
            // Handle xrun
            if (framesRead == -EPIPE) {
                snd_pcm_prepare(inputPCM_);
                snd_pcm_start(inputPCM_);
                continue;
            }
            break;
        }

        // Process audio
        if (callback_) {
            callback_(inputBuffer.data(), outputBuffer.data(), framesRead);
        }
        
        // Update meters
        updateMeters(inputBuffer.data(), outputBuffer.data(), framesRead);

        // Write output
        snd_pcm_sframes_t framesWritten = snd_pcm_writei(outputPCM_, outputBuffer.data(), framesRead);

        if (framesWritten < 0) {
            // Handle xrun
            if (framesWritten == -EPIPE) {
                snd_pcm_prepare(outputPCM_);
                continue;
            }
            break;
        }
    }
}

float ALSABackend::linearToDb(float linear) {
    if (linear <= 0.0f) return -60.0f;
    return 20.0f * std::log10(linear);
}

void ALSABackend::updateMeters(const float* input, const float* output, uint32_t numFrames) {
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

MeterLevels ALSABackend::getInputLevels() const {
    MeterLevels levels;
    levels.leftDb = inputLevelLeft_.load();
    levels.rightDb = inputLevelRight_.load();
    levels.leftPeakDb = inputPeakLeft_.load();
    levels.rightPeakDb = inputPeakRight_.load();
    return levels;
}

MeterLevels ALSABackend::getOutputLevels() const {
    MeterLevels levels;
    levels.leftDb = outputLevelLeft_.load();
    levels.rightDb = outputLevelRight_.load();
    levels.leftPeakDb = outputPeakLeft_.load();
    levels.rightPeakDb = outputPeakRight_.load();
    return levels;
}

void ALSABackend::resetPeakHold() {
    inputPeakLeft_.store(-60.0f);
    inputPeakRight_.store(-60.0f);
    outputPeakLeft_.store(-60.0f);
    outputPeakRight_.store(-60.0f);
}

} // namespace openamp

#endif // USE_ALSA
