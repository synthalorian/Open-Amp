#include "openamp/dsp_engine.h"
#include "openamp/preset_store.h"
#include "noise_gate.h"
#include "compressor.h"
#include "eq.h"
#include "distortion.h"
#include "delay.h"
#include "reverb.h"
#include <cmath>

namespace openamp {

DSPEngine::DSPEngine() {
    // Create amp
    amp_ = std::make_unique<AmpSimulator>();
    
    // Create effects
    noiseGate_ = std::make_unique<NoiseGate>();
    compressor_ = std::make_unique<Compressor>();
    eq_ = std::make_unique<EQ>();
    distortion_ = std::make_unique<Distortion>();
    delay_ = std::make_unique<Delay>();
    reverb_ = std::make_unique<Reverb>();
    
    // Create IR loader
    irLoader_ = std::make_unique<IRLoader>();
    
    // Set default noise gate settings
    noiseGate_->setThreshold(-40.0f);
    noiseGate_->setAttack(1.0f);
    noiseGate_->setRelease(100.0f);
    
    // Set default compressor settings
    compressor_->setThreshold(-20.0f);
    compressor_->setRatio(4.0f);
    compressor_->setAttack(10.0f);
    compressor_->setRelease(100.0f);
    compressor_->setMakeupGain(3.0f);
}

DSPEngine::~DSPEngine() = default;

void DSPEngine::prepare(double sampleRate, uint32_t blockSize) {
    sampleRate_ = sampleRate;
    blockSize_ = blockSize;

    // Prepare all processors
    noiseGate_->prepare(sampleRate, blockSize);
    compressor_->prepare(sampleRate, blockSize);
    eq_->prepare(sampleRate, blockSize);
    distortion_->prepare(sampleRate, blockSize);
    amp_->prepare(sampleRate, blockSize);
    delay_->prepare(sampleRate, blockSize);
    reverb_->prepare(sampleRate, blockSize);
    irLoader_->prepare(sampleRate, blockSize);
}

void DSPEngine::process(float* data, uint32_t numFrames) {
    // Mark input time for latency monitoring
    latencyMonitor_.markInputTime();

    // Create buffer wrapper for single-pass processing
    AudioBuffer buffer;
    buffer.data = data;
    buffer.numChannels = 1;
    buffer.numFrames = numFrames;
    buffer.sampleRate = static_cast<uint32_t>(sampleRate_);

    // Signal chain: Gate → Compressor → EQ → Distortion → Amp → IR → Delay → Reverb
    
    // 1. Noise Gate (first to clean up signal)
    if (noiseGateEnabled_ && noiseGate_) {
        noiseGate_->process(buffer);
    }
    
    // 2. Compressor
    if (compressorEnabled_ && compressor_) {
        compressor_->process(buffer);
    }
    
    // 3. EQ
    if (eqEnabled_ && eq_) {
        eq_->process(buffer);
    }
    
    // 4. Distortion (before amp for typical pedal chain)
    if (distortionEnabled_ && distortion_) {
        distortion_->process(buffer);
    }
    
    // 5. Amp simulation
    if (ampEnabled_ && amp_) {
        amp_->process(buffer);
    }
    
    // 6. IR Cabinet (post-amp, like real cab)
    if (irEnabled_ && irLoader_) {
        irLoader_->process(buffer);
    }
    
    // 7. Delay and Reverb (order depends on delayFirst setting)
    if (delayFirst_) {
        if (delayEnabled_ && delay_) {
            delay_->process(buffer);
        }
        if (reverbEnabled_ && reverb_) {
            reverb_->process(buffer);
        }
    } else {
        if (reverbEnabled_ && reverb_) {
            reverb_->process(buffer);
        }
        if (delayEnabled_ && delay_) {
            delay_->process(buffer);
        }
    }

    // Mark output time for latency monitoring
    latencyMonitor_.markOutputTime();
}

void DSPEngine::reset() {
    if (noiseGate_) noiseGate_->reset();
    if (compressor_) compressor_->reset();
    if (eq_) eq_->reset();
    if (distortion_) distortion_->reset();
    if (amp_) amp_->reset();
    if (delay_) delay_->reset();
    if (reverb_) reverb_->reset();
    if (irLoader_) irLoader_->reset();
    latencyMonitor_.reset();
}

// Amp controls
void DSPEngine::setAmpEnabled(bool enabled) {
    ampEnabled_ = enabled;
}

void DSPEngine::setGain(float gainDb) {
    if (amp_) amp_->setGain(gainDb);
    currentPreset_.ampGainDb = gainDb;
}

void DSPEngine::setDrive(float drive) {
    if (amp_) amp_->setDrive(drive);
    currentPreset_.ampDrive = drive;
}

void DSPEngine::setBass(float db) {
    if (amp_) amp_->setBass(db);
    currentPreset_.ampBassDb = db;
}

void DSPEngine::setMid(float db) {
    if (amp_) amp_->setMid(db);
    currentPreset_.ampMidDb = db;
}

void DSPEngine::setTreble(float db) {
    if (amp_) amp_->setTreble(db);
    currentPreset_.ampTrebleDb = db;
}

void DSPEngine::setPresence(float db) {
    if (amp_) amp_->setPresence(db);
    currentPreset_.ampPresenceDb = db;
}

void DSPEngine::setMaster(float db) {
    if (amp_) amp_->setMaster(db);
    currentPreset_.ampMasterDb = db;
}

// Noise Gate
void DSPEngine::setNoiseGateEnabled(bool enabled) {
    noiseGateEnabled_ = enabled;
}

void DSPEngine::setNoiseGateThreshold(float db) {
    if (noiseGate_) noiseGate_->setThreshold(db);
}

void DSPEngine::setNoiseGateAttack(float ms) {
    if (noiseGate_) noiseGate_->setAttack(ms);
}

void DSPEngine::setNoiseGateRelease(float ms) {
    if (noiseGate_) noiseGate_->setRelease(ms);
}

// Compressor
void DSPEngine::setCompressorEnabled(bool enabled) {
    compressorEnabled_ = enabled;
}

void DSPEngine::setCompressorThreshold(float db) {
    if (compressor_) compressor_->setThreshold(db);
}

void DSPEngine::setCompressorRatio(float ratio) {
    if (compressor_) compressor_->setRatio(ratio);
}

void DSPEngine::setCompressorAttack(float ms) {
    if (compressor_) compressor_->setAttack(ms);
}

void DSPEngine::setCompressorRelease(float ms) {
    if (compressor_) compressor_->setRelease(ms);
}

// EQ
void DSPEngine::setEQEnabled(bool enabled) {
    eqEnabled_ = enabled;
}

void DSPEngine::setEQBand(int band, float db) {
    if (eq_) eq_->setBandGain(band, db);
}

// Effect toggles
void DSPEngine::setDistortionEnabled(bool enabled) {
    distortionEnabled_ = enabled;
    currentPreset_.distortionEnabled = enabled;
}

void DSPEngine::setDelayEnabled(bool enabled) {
    delayEnabled_ = enabled;
    currentPreset_.delayEnabled = enabled;
}

void DSPEngine::setReverbEnabled(bool enabled) {
    reverbEnabled_ = enabled;
    currentPreset_.reverbEnabled = enabled;
}

// Distortion controls
void DSPEngine::setDistortionType(int type) {
    if (distortion_) distortion_->setType(static_cast<Distortion::Type>(type));
    currentPreset_.distortionType = type;
}

void DSPEngine::setDistortionDrive(float drive) {
    if (distortion_) distortion_->setDrive(drive);
    currentPreset_.distortionDrive = drive;
}

void DSPEngine::setDistortionTone(float tone) {
    if (distortion_) distortion_->setTone(tone);
    currentPreset_.distortionTone = tone;
}

void DSPEngine::setDistortionLevel(float level) {
    if (distortion_) distortion_->setLevel(level);
    currentPreset_.distortionLevel = level;
}

// Delay controls
void DSPEngine::setDelayTime(float ms) {
    if (delay_) delay_->setTimeMs(ms);
    currentPreset_.delayTimeMs = ms;
}

void DSPEngine::setDelayFeedback(float feedback) {
    if (delay_) delay_->setFeedback(feedback);
    currentPreset_.delayFeedback = feedback;
}

void DSPEngine::setDelayMix(float mix) {
    if (delay_) delay_->setMix(mix);
    currentPreset_.delayMix = mix;
}

void DSPEngine::setDelayFirst(bool first) {
    delayFirst_ = first;
    currentPreset_.delayFirst = first;
}

// Reverb controls
void DSPEngine::setReverbRoom(float room) {
    if (reverb_) reverb_->setRoomSize(room);
    currentPreset_.reverbRoom = room;
}

void DSPEngine::setReverbDamp(float damp) {
    if (reverb_) reverb_->setDamping(damp);
    currentPreset_.reverbDamp = damp;
}

void DSPEngine::setReverbMix(float mix) {
    if (reverb_) reverb_->setMix(mix);
    currentPreset_.reverbMix = mix;
}

// Presets
bool DSPEngine::loadPreset(const std::string& path, std::string& error) {
    if (PresetStore::loadPreset(path, currentPreset_, error)) {
        applyPreset(currentPreset_);
        return true;
    }
    return false;
}

bool DSPEngine::savePreset(const std::string& path, std::string& error) {
    return PresetStore::savePreset(currentPreset_, path, error);
}

void DSPEngine::applyPreset(const Preset& preset) {
    currentPreset_ = preset;

    // Apply amp settings
    if (amp_) {
        amp_->setGain(preset.ampGainDb);
        amp_->setDrive(preset.ampDrive);
        amp_->setBass(preset.ampBassDb);
        amp_->setMid(preset.ampMidDb);
        amp_->setTreble(preset.ampTrebleDb);
        amp_->setPresence(preset.ampPresenceDb);
        amp_->setMaster(preset.ampMasterDb);
    }
    ampEnabled_ = preset.ampEnabled;

    // Apply distortion settings
    if (distortion_) {
        distortion_->setType(static_cast<Distortion::Type>(preset.distortionType));
        distortion_->setDrive(preset.distortionDrive);
        distortion_->setTone(preset.distortionTone);
        distortion_->setLevel(preset.distortionLevel);
    }
    distortionEnabled_ = preset.distortionEnabled;

    // Apply delay settings
    if (delay_) {
        delay_->setTimeMs(preset.delayTimeMs);
        delay_->setFeedback(preset.delayFeedback);
        delay_->setMix(preset.delayMix);
    }
    delayEnabled_ = preset.delayEnabled;
    delayFirst_ = preset.delayFirst;

    // Apply reverb settings
    if (reverb_) {
        reverb_->setRoomSize(preset.reverbRoom);
        reverb_->setDamping(preset.reverbDamp);
        reverb_->setMix(preset.reverbMix);
    }
    reverbEnabled_ = preset.reverbEnabled;
}

// IR Loader methods
bool DSPEngine::loadIR(const std::string& path, std::string& error) {
    if (irLoader_) {
        return irLoader_->loadIR(path, error);
    }
    error = "IR Loader not initialized";
    return false;
}

void DSPEngine::setIREnabled(bool enabled) {
    irEnabled_ = enabled;
}

void DSPEngine::setIRMix(float mix) {
    if (irLoader_) irLoader_->setMix(mix);
}

void DSPEngine::setIRInputGain(float gainDb) {
    if (irLoader_) irLoader_->setInputGain(gainDb);
}

void DSPEngine::setIROutputGain(float gainDb) {
    if (irLoader_) irLoader_->setOutputGain(gainDb);
}

void DSPEngine::setIRHighCut(float hz) {
    if (irLoader_) irLoader_->setHighCut(hz);
}

void DSPEngine::setIRLowCut(float hz) {
    if (irLoader_) irLoader_->setLowCut(hz);
}

std::string DSPEngine::getIRName() const {
    if (irLoader_) return irLoader_->getIRName();
    return "";
}

float DSPEngine::getIRCPUsage() const {
    if (irLoader_) return irLoader_->getCurrentCPU();
    return 0.0f;
}

// Latency monitoring methods
float DSPEngine::getLatencyMs() const {
    return latencyMonitor_.getLatencyMs();
}

float DSPEngine::getTheoreticalLatencyMs() const {
    return LatencyMonitor::calculateTheoreticalLatencyMs(blockSize_, sampleRate_);
}

} // namespace openamp
