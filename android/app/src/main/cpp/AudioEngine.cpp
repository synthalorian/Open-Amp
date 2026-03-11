#include "AudioEngine.h"
#include "openamp/amp_simulator.h"
#include "openamp/effect_chain.h"
#include "openamp/ir_loader.h"
#include "openamp/latency_monitor.h"
#include <android/log.h>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>
#include <stdexcept>

#define LOG_TAG "OpenAmp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace openamp_android {

AudioEngine::AudioEngine() {
    LOGI("AudioEngine: Constructor starting");

    try {
        config_.sampleRate = 48000.0;
        config_.bufferSize = 256;
        config_.numInputChannels = 1;
        config_.numOutputChannels = 2;
        config_.inputSource = openamp::InputSource::USB;
        config_.enableMonitoring = true;

        // Create input processor first
        processor_ = std::make_unique<openamp::InputProcessor>();
        if (!processor_) {
            LOGE("AudioEngine: Failed to create InputProcessor");
            return;
        }
        processor_->initialize(config_);

        // Create amp simulator
        auto amp = std::make_unique<openamp::AmpSimulator>();
        if (!amp) {
            LOGE("AudioEngine: Failed to create AmpSimulator");
            return;
        }
        amp->prepare(config_.sampleRate, config_.bufferSize);
        amp_ = amp.get();
        amp_->setGain(ampGainDb_);
        amp_->setDrive(ampDrive_);
        amp_->setBass(ampBassDb_);
        amp_->setMid(ampMidDb_);
        amp_->setTreble(ampTrebleDb_);
        amp_->setPresence(ampPresenceDb_);
        amp_->setMaster(ampMasterDb_);

        // Create effect chain
        auto chain = std::make_unique<openamp::EffectChain>();
        if (!chain) {
            LOGE("AudioEngine: Failed to create EffectChain");
            return;
        }
        chain->prepare(config_.sampleRate, config_.bufferSize);

        // Create distortion
        auto distortion = std::make_unique<openamp::Distortion>();
        if (!distortion) {
            LOGE("AudioEngine: Failed to create Distortion");
            return;
        }
        distortion->prepare(config_.sampleRate, config_.bufferSize);
        distortion->setDrive(distortionDrive_);
        distortion->setTone(distortionTone_);
        distortion->setLevel(distortionLevel_);
        distortion->setType(static_cast<openamp::Distortion::Type>(distortionType_));
        distortion_ = distortion.get();

        // Create delay
        auto delay = std::make_unique<openamp::Delay>();
        if (!delay) {
            LOGE("AudioEngine: Failed to create Delay");
            return;
        }
        delay->prepare(config_.sampleRate, config_.bufferSize);
        delay->setTimeMs(delayTimeMs_);
        delay->setFeedback(delayFeedback_);
        delay->setMix(delayMix_);
        delay_ = delay.get();

        // Create reverb
        auto reverb = std::make_unique<openamp::Reverb>();
        if (!reverb) {
            LOGE("AudioEngine: Failed to create Reverb");
            return;
        }
        reverb->prepare(config_.sampleRate, config_.bufferSize);
        reverb->setRoomSize(reverbRoom_);
        reverb->setDamping(reverbDamp_);
        reverb->setMix(reverbMix_);
        reverb_ = reverb.get();

        // Add effects to chain
        chain->addEffect(std::move(distortion));
        chain->addEffect(std::move(delay));
        chain->addEffect(std::move(reverb));

        chain->setEffectEnabled(0, distortionEnabled_);
        chain->setEffectEnabled(1, delayEnabled_);
        chain->setEffectEnabled(2, reverbEnabled_);

        chain_ = chain.get();

        processor_->setAmpSimulator(std::move(amp));
        processor_->setEffectChain(std::move(chain));

        // Create IR Loader (NEW)
        irLoader_ = std::make_unique<openamp::IRLoader>();
        if (irLoader_) {
            irLoader_->prepare(config_.sampleRate, config_.bufferSize);
        }

        // USB interfaces typically need more input gain
  // Guitar pickups are often lower line level than mic input
        processor_->setInputGain(36.0f);
        processor_->setOutputGain(12.0f);

        inputBuffer_.resize(config_.bufferSize * 4);
        outputBuffer_.resize(config_.bufferSize * config_.numOutputChannels * 4);
        inputInterleavedBuffer_.resize(config_.bufferSize * 4);

        LOGI("AudioEngine: Constructor complete");
    } catch (const std::exception& e) {
        LOGE("AudioEngine: Exception in constructor: %s", e.what());
    } catch (...) {
        LOGE("AudioEngine: Unknown exception in constructor");
    }
}

AudioEngine::~AudioEngine() {
    LOGI("AudioEngine: Destructor");
    try {
        stop();
    } catch (...) {
        LOGE("AudioEngine: Exception in destructor");
    }
}

bool AudioEngine::start() {
    LOGI("AudioEngine: start() called");
    
    try {
        if (running_) {
            LOGI("AudioEngine: Already running");
            return true;
        }
        
        if (!processor_) {
            LOGE("AudioEngine: Cannot start - processor is null");
            return false;
        }
        
        if (!openStreams()) {
            LOGE("AudioEngine: Failed to open streams");
            return false;
        }

        if (inputStream_) {
            oboe::Result result = inputStream_->requestStart();
            if (result != oboe::Result::OK) {
                LOGE("AudioEngine: Failed to start input stream: %d", static_cast<int>(result));
                closeStreams();
                return false;
            }
            LOGI("AudioEngine: Input stream started");
        }

        if (outputStream_) {
            oboe::Result result = outputStream_->requestStart();
            if (result != oboe::Result::OK) {
                LOGE("AudioEngine: Failed to start output stream: %d", static_cast<int>(result));
                if (inputStream_) inputStream_->requestStop();
                closeStreams();
                return false;
            }
            LOGI("AudioEngine: Output stream started");
        }

        running_.store(true);
        LOGI("AudioEngine: Started successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("AudioEngine: Exception in start(): %s", e.what());
        return false;
    } catch (...) {
        LOGE("AudioEngine: Unknown exception in start()");
        return false;
    }
}

void AudioEngine::stop() {
    if (!running_) return;
    running_.store(false);
    if (outputStream_) outputStream_->requestStop();
    if (inputStream_) inputStream_->requestStop();
    closeStreams();
}

// IR Loader methods (NEW)
bool AudioEngine::loadIRFromWavFile(const std::string& path) {
    if (!irLoader_) return false;
    std::string error;
    return irLoader_->loadIR(path, error);
}

void AudioEngine::setIREnabled(bool enabled) {
    irEnabled_ = enabled;
    if (irLoader_) irLoader_->setEnabled(enabled);
}

void AudioEngine::setIRMix(float mix) {
    irMix_ = mix;
    if (irLoader_) irLoader_->setMix(mix);
}

void AudioEngine::setIRInputGain(float db) {
    if (irLoader_) irLoader_->setInputGain(db);
}

void AudioEngine::setIROutputGain(float db) {
    if (irLoader_) irLoader_->setOutputGain(db);
}

void AudioEngine::setIRHighCut(float hz) {
    if (irLoader_) irLoader_->setHighCut(hz);
}

void AudioEngine::setIRLowCut(float hz) {
    if (irLoader_) irLoader_->setLowCut(hz);
}

bool AudioEngine::isIRLoaded() const {
    return irLoader_ && irLoader_->isIRLoaded();
}

std::string AudioEngine::getIRName() const {
    return irLoader_ ? irLoader_->getIRName() : "";
}

float AudioEngine::getIRCPUsage() const {
    return irLoader_ ? irLoader_->getCurrentCPU() : 0.0f;
}

bool AudioEngine::getIREnabled() const {
    return irEnabled_;
}

float AudioEngine::getIRMix() const {
    return irMix_;
}

// Latency monitoring (NEW)
float AudioEngine::getLatencyMs() const {
    return latencyMonitor_.getLatencyMs();
}

float AudioEngine::getTheoreticalLatencyMs() const {
    return openamp::LatencyMonitor::calculateTheoreticalLatencyMs(
        config_.bufferSize, config_.sampleRate);
}

// Gain controls
void AudioEngine::setInputGain(float db) { if (processor_) processor_->setInputGain(db); }
void AudioEngine::setOutputGain(float db) { if (processor_) processor_->setOutputGain(db); }
float AudioEngine::getInputGainDb() const { return processor_ ? processor_->getInputGainDb() : 0.0f; }
float AudioEngine::getOutputGainDb() const { return processor_ ? processor_->getOutputGainDb() : 0.0f; }

// Amp controls
void AudioEngine::setAmpEnabled(bool enabled) { if (processor_) processor_->setAmpEnabled(enabled); }
void AudioEngine::setAmpGainDb(float db) { ampGainDb_ = db; if (amp_) amp_->setGain(db); }
void AudioEngine::setAmpDrive(float amount) { ampDrive_ = amount; if (amp_) amp_->setDrive(amount); }
void AudioEngine::setAmpBassDb(float db) { ampBassDb_ = db; if (amp_) amp_->setBass(db); }
void AudioEngine::setAmpMidDb(float db) { ampMidDb_ = db; if (amp_) amp_->setMid(db); }
void AudioEngine::setAmpTrebleDb(float db) { ampTrebleDb_ = db; if (amp_) amp_->setTreble(db); }
void AudioEngine::setAmpPresenceDb(float db) { ampPresenceDb_ = db; if (amp_) amp_->setPresence(db); }
void AudioEngine::setAmpMasterDb(float db) { ampMasterDb_ = db; if (amp_) amp_->setMaster(db); }

bool AudioEngine::getAmpEnabled() const { return processor_ ? processor_->isAmpEnabled() : false; }
float AudioEngine::getAmpGainDb() const { return ampGainDb_; }
float AudioEngine::getAmpDrive() const { return ampDrive_; }
float AudioEngine::getAmpBassDb() const { return ampBassDb_; }
float AudioEngine::getAmpMidDb() const { return ampMidDb_; }
float AudioEngine::getAmpTrebleDb() const { return ampTrebleDb_; }
float AudioEngine::getAmpPresenceDb() const { return ampPresenceDb_; }
float AudioEngine::getAmpMasterDb() const { return ampMasterDb_; }

// Effects
void AudioEngine::setEffectsEnabled(bool enabled) { if (processor_) processor_->setEffectsEnabled(enabled); }
bool AudioEngine::getEffectsEnabled() const { return processor_ ? processor_->isEffectsEnabled() : false; }

// Distortion
void AudioEngine::setDistortionEnabled(bool enabled) { distortionEnabled_ = enabled; if (chain_) chain_->setEffectEnabled(0, enabled); }
void AudioEngine::setDistortionType(int type) { distortionType_ = type; if (distortion_) distortion_->setType(static_cast<openamp::Distortion::Type>(type)); }
void AudioEngine::setDistortionDrive(float amount) { distortionDrive_ = amount; if (distortion_) distortion_->setDrive(amount); }
void AudioEngine::setDistortionTone(float amount) { distortionTone_ = amount; if (distortion_) distortion_->setTone(amount); }
void AudioEngine::setDistortionLevel(float amount) { distortionLevel_ = amount; if (distortion_) distortion_->setLevel(amount); }
bool AudioEngine::getDistortionEnabled() const { return distortionEnabled_; }
int AudioEngine::getDistortionType() const { return distortionType_; }
float AudioEngine::getDistortionDrive() const { return distortionDrive_; }
float AudioEngine::getDistortionTone() const { return distortionTone_; }
float AudioEngine::getDistortionLevel() const { return distortionLevel_; }

// Delay
void AudioEngine::setDelayEnabled(bool enabled) { delayEnabled_ = enabled; if (chain_) chain_->setEffectEnabled(1, enabled); }
void AudioEngine::setDelayTimeMs(float ms) { delayTimeMs_ = ms; if (delay_) delay_->setTimeMs(ms); }
void AudioEngine::setDelayFeedback(float amount) { delayFeedback_ = amount; if (delay_) delay_->setFeedback(amount); }
void AudioEngine::setDelayMix(float amount) { delayMix_ = amount; if (delay_) delay_->setMix(amount); }
bool AudioEngine::getDelayEnabled() const { return delayEnabled_; }
float AudioEngine::getDelayTimeMs() const { return delayTimeMs_; }
float AudioEngine::getDelayFeedback() const { return delayFeedback_; }
float AudioEngine::getDelayMix() const { return delayMix_; }

// Reverb
void AudioEngine::setReverbEnabled(bool enabled) { reverbEnabled_ = enabled; if (chain_) chain_->setEffectEnabled(2, enabled); }
void AudioEngine::setReverbRoomSize(float amount) { reverbRoom_ = amount; if (reverb_) reverb_->setRoomSize(amount); }
void AudioEngine::setReverbDamping(float amount) { reverbDamp_ = amount; if (reverb_) reverb_->setDamping(amount); }
void AudioEngine::setReverbMix(float amount) { reverbMix_ = amount; if (reverb_) reverb_->setMix(amount); }
bool AudioEngine::getReverbEnabled() const { return reverbEnabled_; }
float AudioEngine::getReverbRoom() const { return reverbRoom_; }
float AudioEngine::getReverbDamp() const { return reverbDamp_; }
float AudioEngine::getReverbMix() const { return reverbMix_; }

void AudioEngine::setDelayFirst(bool enabled) { delayFirst_ = enabled; }
bool AudioEngine::getDelayFirst() const { return delayFirst_; }

// Device selection
void AudioEngine::setInputDeviceId(int32_t deviceId) { inputDeviceId_ = deviceId; }
void AudioEngine::setOutputDeviceId(int32_t deviceId) { outputDeviceId_ = deviceId; }

// Meters
float AudioEngine::getInputLevel() const { return processor_ ? processor_->getInputLevel() : 0.0f; }
float AudioEngine::getOutputLevel() const { return processor_ ? processor_->getOutputLevel() : 0.0f; }
bool AudioEngine::getClipping() const { return processor_ ? processor_->isClipping() : false; }
void AudioEngine::resetClipIndicator() { if (processor_) processor_->resetClipIndicator(); }
void AudioEngine::setInputChannelMode(int mode) { inputChannelMode_ = std::max(0, std::min(3, mode)); }
void AudioEngine::setTestToneEnabled(bool enabled) { testToneEnabled_ = enabled; }

// Presets
bool AudioEngine::savePreset(const std::string& path, const std::string& name) {
    if (!processor_) return false;
    openamp::Preset preset;
    preset.name = name;
    preset.inputGainDb = processor_->getInputGainDb();
    preset.outputGainDb = processor_->getOutputGainDb();
    preset.ampEnabled = processor_->isAmpEnabled();
    preset.effectsEnabled = processor_->isEffectsEnabled();
    preset.delayEnabled = delayEnabled_;
    preset.reverbEnabled = reverbEnabled_;
    preset.distortionEnabled = distortionEnabled_;
    preset.delayFirst = delayFirst_;
    preset.delayTimeMs = delayTimeMs_;
    preset.delayFeedback = delayFeedback_;
    preset.delayMix = delayMix_;
    preset.reverbRoom = reverbRoom_;
    preset.reverbDamp = reverbDamp_;
    preset.reverbMix = reverbMix_;
    preset.distortionDrive = distortionDrive_;
    preset.distortionTone = distortionTone_;
    preset.distortionLevel = distortionLevel_;
    preset.distortionType = distortionType_;
    preset.ampGainDb = ampGainDb_;
    preset.ampDrive = ampDrive_;
    preset.ampBassDb = ampBassDb_;
    preset.ampMidDb = ampMidDb_;
    preset.ampTrebleDb = ampTrebleDb_;
    preset.ampPresenceDb = ampPresenceDb_;
    preset.ampMasterDb = ampMasterDb_;
    preset.cabIrPath = cabIrPath_;
    std::string error;
    return openamp::PresetStore::savePreset(preset, path, error);
}

bool AudioEngine::loadPreset(const std::string& path) {
    openamp::Preset preset;
    std::string error;
    if (!openamp::PresetStore::loadPreset(path, preset, error)) return false;
    applyPreset(preset);
    return true;
}

// Legacy cabinet IR
bool AudioEngine::setCabIRFromFile(const std::string& path) {
    if (!amp_) return false;
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::vector<float> ir;
    ir.reserve(256);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        try { ir.push_back(std::stof(line)); } catch (...) {}
        if (ir.size() >= 256) break;
    }
    if (ir.empty()) return false;
    amp_->setCabIR(ir);
    cabIrPath_ = path;
    return true;
}

// Debug
std::string AudioEngine::getDebugStatus() const {
    std::ostringstream ss;
    ss << "running=" << (running_.load() ? 1 : 0)
       << " cb=" << callbackCount_.load()
       << " inLvl=" << getInputLevel()
       << " outLvl=" << getOutputLevel();
    return ss.str();
}

void AudioEngine::applyPreset(const openamp::Preset& preset) {
    if (processor_) {
        processor_->setInputGain(preset.inputGainDb);
        processor_->setOutputGain(preset.outputGainDb);
        processor_->setAmpEnabled(preset.ampEnabled);
        processor_->setEffectsEnabled(preset.effectsEnabled);
    }
    setDistortionEnabled(preset.distortionEnabled);
    setDistortionType(preset.distortionType);
    setDistortionDrive(preset.distortionDrive);
    setDistortionTone(preset.distortionTone);
    setDistortionLevel(preset.distortionLevel);
    setDelayEnabled(preset.delayEnabled);
    setDelayFirst(preset.delayFirst);
    setDelayTimeMs(preset.delayTimeMs);
    setDelayFeedback(preset.delayFeedback);
    setDelayMix(preset.delayMix);
    setReverbEnabled(preset.reverbEnabled);
    setReverbRoomSize(preset.reverbRoom);
    setReverbDamping(preset.reverbDamp);
    setReverbMix(preset.reverbMix);
    setAmpGainDb(preset.ampGainDb);
    setAmpDrive(preset.ampDrive);
    setAmpBassDb(preset.ampBassDb);
    setAmpMidDb(preset.ampMidDb);
    setAmpTrebleDb(preset.ampTrebleDb);
    setAmpPresenceDb(preset.ampPresenceDb);
    setAmpMasterDb(preset.ampMasterDb);
    if (!preset.cabIrPath.empty()) setCabIRFromFile(preset.cabIrPath);
}

oboe::DataCallbackResult AudioEngine::onAudioReady(
    oboe::AudioStream* audioStream, void* audioData, int32_t numFrames) {

    if (!running_) return oboe::DataCallbackResult::Stop;

    callbackCount_.fetch_add(1);
    lastFramesRequested_.store(numFrames);

    latencyMonitor_.markInputTime();

    float* output = static_cast<float*>(audioData);
    const int32_t numOutputSamples = numFrames * config_.numOutputChannels;
    const int32_t inputChannels = inputStream_ ? inputStream_->getChannelCount() : 1;
    const int32_t numInputSamples = numFrames * std::max(1, inputChannels);

    if (static_cast<size_t>(numFrames) > inputBuffer_.size()) inputBuffer_.resize(numFrames);
    if (static_cast<size_t>(numInputSamples) > inputInterleavedBuffer_.size()) inputInterleavedBuffer_.resize(numInputSamples);
    if (static_cast<size_t>(numOutputSamples) > outputBuffer_.size()) outputBuffer_.resize(numOutputSamples);

    int32_t framesRead = 0;
    if (inputStream_) {
        auto readResult = inputStream_->read(inputInterleavedBuffer_.data(), numFrames, 0);
        if (readResult) {
            framesRead = readResult.value();
            inputReadCount_.fetch_add(1);
        } else if (readResult.error() == oboe::Result::ErrorTimeout) {
            inputTimeoutCount_.fetch_add(1);
        } else {
            inputErrorCount_.fetch_add(1);
        }
    }
    lastFramesRead_.store(framesRead);

    const int32_t safeChannels = std::max(1, inputChannels);
    for (int32_t i = 0; i < framesRead; ++i) {
        const int32_t base = i * safeChannels;
        float mono = 0.0f;
        if (safeChannels == 1) {
            mono = inputInterleavedBuffer_[base];
        } else {
            float left = inputInterleavedBuffer_[base];
            float right = inputInterleavedBuffer_[base + 1];
            if (inputChannelMode_ == 0) mono = left;
            else if (inputChannelMode_ == 1) mono = right;
            else if (inputChannelMode_ == 2) mono = 0.5f * (left + right);
            else mono = (std::fabs(left) >= std::fabs(right)) ? left : right;
        }
        inputBuffer_[i] = mono;
    }
    for (int32_t i = framesRead; i < numFrames; ++i) inputBuffer_[i] = 0.0f;

    if (testToneEnabled_) {
        const float freq = 440.0f;
        const float phaseInc = 2.0f * static_cast<float>(M_PI) * freq / static_cast<float>(config_.sampleRate);
        for (int32_t i = 0; i < numFrames; ++i) {
            float s = 0.08f * std::sin(testTonePhase_);
            testTonePhase_ += phaseInc;
            if (testTonePhase_ > 2.0f * static_cast<float>(M_PI)) testTonePhase_ -= 2.0f * static_cast<float>(M_PI);
            for (uint32_t ch = 0; ch < config_.numOutputChannels; ++ch)
                outputBuffer_[i * config_.numOutputChannels + ch] = s;
        }
    } else if (processor_) {
        processor_->processInput(inputBuffer_.data(), outputBuffer_.data(), static_cast<uint32_t>(numFrames));
    } else {
        for (int32_t i = 0; i < numFrames; ++i) {
            for (uint32_t ch = 0; ch < config_.numOutputChannels; ++ch)
                outputBuffer_[i * config_.numOutputChannels + ch] = inputBuffer_[i];
        }
    }

    // Process IR Loader if enabled and loaded
    if (irEnabled_ && irLoader_ && irLoader_->isIRLoaded()) {
        openamp::AudioBuffer buffer;
        buffer.data = outputBuffer_.data();
        buffer.numChannels = config_.numOutputChannels;
        buffer.numFrames = numFrames;
        buffer.sampleRate = static_cast<uint32_t>(config_.sampleRate);
        irLoader_->process(buffer);
    }

    memcpy(output, outputBuffer_.data(), numOutputSamples * sizeof(float));

    latencyMonitor_.markOutputTime();
    return oboe::DataCallbackResult::Continue;
}

void AudioEngine::onErrorBeforeClose(oboe::AudioStream*, oboe::Result error) {
    LOGE("AudioEngine: Stream error before close: %d", static_cast<int>(error));
}

void AudioEngine::onErrorAfterClose(oboe::AudioStream*, oboe::Result error) {
    LOGE("AudioEngine: Stream error after close: %d", static_cast<int>(error));
    if (!running_) return;
    if (restartRequested_.exchange(true)) return;
    std::thread([this]() {
        std::lock_guard<std::mutex> lock(restartMutex_);
        if (!running_) { restartRequested_ = false; return; }
        stop();
        start();
        restartRequested_ = false;
    }).detach();
}

bool AudioEngine::openStreams() {
    LOGI("AudioEngine: openStreams() called");
    
    // Configure output stream
    outputBuilder_.setDirection(oboe::Direction::Output)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Shared)
        ->setChannelCount(config_.numOutputChannels)
        ->setFormat(oboe::AudioFormat::Float)
        ->setUsage(oboe::Usage::Media)
        ->setContentType(oboe::ContentType::Music)
        ->setDataCallback(this)
        ->setErrorCallback(this);
    
    if (outputDeviceId_ >= 0) {
        outputBuilder_.setDeviceId(outputDeviceId_);
        LOGI("AudioEngine: Using output device %d", outputDeviceId_);
    }

    oboe::Result result = outputBuilder_.openStream(outputStream_);
    if (result != oboe::Result::OK) {
        LOGE("AudioEngine: Failed to open output stream: %d", static_cast<int>(result));
        return false;
    }
    
    if (!outputStream_) {
        LOGE("AudioEngine: Output stream is null after open");
        return false;
    }
    
    config_.sampleRate = outputStream_->getSampleRate();
    LOGI("AudioEngine: Output stream opened, sample rate: %f", config_.sampleRate);

    // Configure input stream
    inputBuilder_.setDirection(oboe::Direction::Input)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Shared)
        ->setChannelCount(config_.numInputChannels)
        ->setFormat(oboe::AudioFormat::Float)
        ->setInputPreset(oboe::InputPreset::Unprocessed)
        ->setSampleRate(config_.sampleRate)
        ->setUsage(oboe::Usage::Media)
        ->setErrorCallback(this);
    
    if (inputDeviceId_ >= 0) {
        inputBuilder_.setDeviceId(inputDeviceId_);
        LOGI("AudioEngine: Using input device %d", inputDeviceId_);
    }

    result = inputBuilder_.openStream(inputStream_);
    if (result != oboe::Result::OK) {
        LOGW("AudioEngine: Failed to open input stream with device %d: %d", inputDeviceId_, static_cast<int>(result));
        // Try with default device
        inputBuilder_.setDeviceId(oboe::Unspecified);
        result = inputBuilder_.openStream(inputStream_);
        if (result != oboe::Result::OK) {
            LOGE("AudioEngine: Failed to open input stream (default): %d", static_cast<int>(result));
            if (outputStream_) { 
                outputStream_->close(); 
                outputStream_.reset(); 
            }
            return false;
        }
        LOGI("AudioEngine: Input stream opened with default device");
    } else {
        LOGI("AudioEngine: Input stream opened successfully");
    }

    // Reinitialize processor with actual config
    if (processor_) {
        LOGI("AudioEngine: Reinitializing processor with sample rate %f, buffer size %u", 
             config_.sampleRate, config_.bufferSize);
        processor_->initialize(config_);
        
        // Re-prepare IR loader with actual sample rate
        if (irLoader_) {
            irLoader_->prepare(config_.sampleRate, config_.bufferSize);
        }
    }
    
    LOGI("AudioEngine: openStreams() succeeded");
    return true;
}

void AudioEngine::closeStreams() {
    inputStream_.reset();
    outputStream_.reset();
}

} // namespace openamp_android
