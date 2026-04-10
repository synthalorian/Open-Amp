#include "openamp/dsp_engine.h"
#include "openamp/dsp_constants.h"
#include "openamp/preset_store.h"
#include "noise_gate.h"
#include "compressor.h"
#include "eq.h"
#include "distortion.h"
#include "delay.h"
#include "reverb.h"
#include <algorithm>
#include <cmath>

namespace openamp {

using namespace constants;

DSPEngine::DSPEngine() {
    amp_ = std::make_unique<AmpSimulator>();
    noiseGate_ = std::make_unique<NoiseGate>();
    compressor_ = std::make_unique<Compressor>();
    eq_ = std::make_unique<EQ>();
    distortion_ = std::make_unique<Distortion>();
    delay_ = std::make_unique<Delay>();
    reverb_ = std::make_unique<Reverb>();
    irLoader_ = std::make_unique<IRLoader>();

    noiseGate_->setThreshold(kDefaultNoiseGateThreshold);
    noiseGate_->setAttack(kDefaultNoiseGateAttack);
    noiseGate_->setRelease(kDefaultNoiseGateRelease);

    compressor_->setThreshold(kDefaultCompressorThreshold);
    compressor_->setRatio(kDefaultCompressorRatio);
    compressor_->setAttack(kDefaultCompressorAttack);
    compressor_->setRelease(kDefaultCompressorRelease);
    compressor_->setMakeupGain(kDefaultCompressorMakeupGain);
}

DSPEngine::~DSPEngine() = default;

void DSPEngine::prepare(double sampleRate, uint32_t blockSize) {
    sampleRate_ = std::clamp(sampleRate, kMinSampleRate, kMaxSampleRate);
    blockSize_ = std::clamp(blockSize, kMinBlockSize, kMaxBlockSize);

    noiseGate_->prepare(sampleRate_, blockSize_);
    compressor_->prepare(sampleRate_, blockSize_);
    eq_->prepare(sampleRate_, blockSize_);
    distortion_->prepare(sampleRate_, blockSize_);
    amp_->prepare(sampleRate_, blockSize_);
    delay_->prepare(sampleRate_, blockSize_);
    reverb_->prepare(sampleRate_, blockSize_);
    irLoader_->prepare(sampleRate_, blockSize_);
}

void DSPEngine::process(float* data, uint32_t numFrames) {
    if (!data || numFrames == 0) return;

    latencyMonitor_.markInputTime();

    AudioBuffer buffer;
    buffer.data = data;
    buffer.numChannels = 1;
    buffer.numFrames = numFrames;
    buffer.sampleRate = static_cast<uint32_t>(sampleRate_);

    if (noiseGateEnabled_ && noiseGate_) noiseGate_->process(buffer);
    if (compressorEnabled_ && compressor_) compressor_->process(buffer);
    if (eqEnabled_ && eq_) eq_->process(buffer);
    if (distortionEnabled_ && distortion_) distortion_->process(buffer);
    if (ampEnabled_ && amp_) amp_->process(buffer);
    if (irEnabled_ && irLoader_) irLoader_->process(buffer);

    if (delayFirst_) {
        if (delayEnabled_ && delay_) delay_->process(buffer);
        if (reverbEnabled_ && reverb_) reverb_->process(buffer);
    } else {
        if (reverbEnabled_ && reverb_) reverb_->process(buffer);
        if (delayEnabled_ && delay_) delay_->process(buffer);
    }

    // Final safety limiter to prevent digital clipping
    for (uint32_t i = 0; i < numFrames; ++i) {
        data[i] = std::clamp(data[i], -1.0f, 1.0f);
    }

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

// Amp controls — all clamped
void DSPEngine::setAmpEnabled(bool enabled) { ampEnabled_ = enabled; }

void DSPEngine::setGain(float gainDb) {
    gainDb = std::clamp(gainDb, kMinGainDb, kMaxGainDb);
    if (amp_) amp_->setGain(gainDb);
    currentPreset_.ampGainDb = gainDb;
}

void DSPEngine::setDrive(float drive) {
    drive = std::clamp(drive, kMinDrive, kMaxDrive);
    if (amp_) amp_->setDrive(drive);
    currentPreset_.ampDrive = drive;
}

void DSPEngine::setBass(float db) {
    db = std::clamp(db, kMinEQDb, kMaxEQDb);
    if (amp_) amp_->setBass(db);
    currentPreset_.ampBassDb = db;
}

void DSPEngine::setMid(float db) {
    db = std::clamp(db, kMinEQDb, kMaxEQDb);
    if (amp_) amp_->setMid(db);
    currentPreset_.ampMidDb = db;
}

void DSPEngine::setTreble(float db) {
    db = std::clamp(db, kMinEQDb, kMaxEQDb);
    if (amp_) amp_->setTreble(db);
    currentPreset_.ampTrebleDb = db;
}

void DSPEngine::setPresence(float db) {
    db = std::clamp(db, kMinEQDb, kMaxEQDb);
    if (amp_) amp_->setPresence(db);
    currentPreset_.ampPresenceDb = db;
}

void DSPEngine::setMaster(float db) {
    db = std::clamp(db, kMinMasterDb, kMaxMasterDb);
    if (amp_) amp_->setMaster(db);
    currentPreset_.ampMasterDb = db;
}

// Noise Gate
void DSPEngine::setNoiseGateEnabled(bool enabled) { noiseGateEnabled_ = enabled; }

void DSPEngine::setNoiseGateThreshold(float db) {
    db = std::clamp(db, kMinNoiseGateThreshold, kMaxNoiseGateThreshold);
    if (noiseGate_) noiseGate_->setThreshold(db);
}

void DSPEngine::setNoiseGateAttack(float ms) {
    ms = std::clamp(ms, kMinAttackMs, kMaxAttackMs);
    if (noiseGate_) noiseGate_->setAttack(ms);
}

void DSPEngine::setNoiseGateRelease(float ms) {
    ms = std::clamp(ms, kMinReleaseMs, kMaxReleaseMs);
    if (noiseGate_) noiseGate_->setRelease(ms);
}

// Compressor
void DSPEngine::setCompressorEnabled(bool enabled) { compressorEnabled_ = enabled; }

void DSPEngine::setCompressorThreshold(float db) {
    db = std::clamp(db, -60.0f, 0.0f);
    if (compressor_) compressor_->setThreshold(db);
}

void DSPEngine::setCompressorRatio(float ratio) {
    ratio = std::clamp(ratio, kMinCompressorRatio, kMaxCompressorRatio);
    if (compressor_) compressor_->setRatio(ratio);
}

void DSPEngine::setCompressorAttack(float ms) {
    ms = std::clamp(ms, kMinAttackMs, kMaxAttackMs);
    if (compressor_) compressor_->setAttack(ms);
}

void DSPEngine::setCompressorRelease(float ms) {
    ms = std::clamp(ms, kMinReleaseMs, kMaxReleaseMs);
    if (compressor_) compressor_->setRelease(ms);
}

// EQ
void DSPEngine::setEQEnabled(bool enabled) { eqEnabled_ = enabled; }

void DSPEngine::setEQBand(int band, float db) {
    db = std::clamp(db, kMinEQDb, kMaxEQDb);
    if (eq_) eq_->setBandGain(band, db);
}

// Distortion
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

void DSPEngine::setDistortionType(int type) {
    type = std::clamp(type, 0, 3);
    if (distortion_) distortion_->setType(static_cast<Distortion::Type>(type));
    currentPreset_.distortionType = type;
}

void DSPEngine::setDistortionDrive(float drive) {
    drive = std::clamp(drive, kMinDrive, kMaxDrive);
    if (distortion_) distortion_->setDrive(drive);
    currentPreset_.distortionDrive = drive;
}

void DSPEngine::setDistortionTone(float tone) {
    tone = std::clamp(tone, kMinMix, kMaxMix);
    if (distortion_) distortion_->setTone(tone);
    currentPreset_.distortionTone = tone;
}

void DSPEngine::setDistortionLevel(float level) {
    level = std::clamp(level, kMinMix, kMaxMix);
    if (distortion_) distortion_->setLevel(level);
    currentPreset_.distortionLevel = level;
}

// Delay
void DSPEngine::setDelayTime(float ms) {
    ms = std::clamp(ms, kMinDelayMs, kMaxDelayMs);
    if (delay_) delay_->setTimeMs(ms);
    currentPreset_.delayTimeMs = ms;
}

void DSPEngine::setDelayFeedback(float feedback) {
    feedback = std::clamp(feedback, kMinFeedback, kMaxFeedback);
    if (delay_) delay_->setFeedback(feedback);
    currentPreset_.delayFeedback = feedback;
}

void DSPEngine::setDelayMix(float mix) {
    mix = std::clamp(mix, kMinMix, kMaxMix);
    if (delay_) delay_->setMix(mix);
    currentPreset_.delayMix = mix;
}

void DSPEngine::setDelayFirst(bool first) {
    delayFirst_ = first;
    currentPreset_.delayFirst = first;
}

// Reverb
void DSPEngine::setReverbRoom(float room) {
    room = std::clamp(room, kMinRoomSize, kMaxRoomSize);
    if (reverb_) reverb_->setRoomSize(room);
    currentPreset_.reverbRoom = room;
}

void DSPEngine::setReverbDamp(float damp) {
    damp = std::clamp(damp, kMinDamping, kMaxDamping);
    if (reverb_) reverb_->setDamping(damp);
    currentPreset_.reverbDamp = damp;
}

void DSPEngine::setReverbMix(float mix) {
    mix = std::clamp(mix, kMinMix, kMaxMix);
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

    if (distortion_) {
        distortion_->setType(static_cast<Distortion::Type>(preset.distortionType));
        distortion_->setDrive(preset.distortionDrive);
        distortion_->setTone(preset.distortionTone);
        distortion_->setLevel(preset.distortionLevel);
    }
    distortionEnabled_ = preset.distortionEnabled;

    if (delay_) {
        delay_->setTimeMs(preset.delayTimeMs);
        delay_->setFeedback(preset.delayFeedback);
        delay_->setMix(preset.delayMix);
    }
    delayEnabled_ = preset.delayEnabled;
    delayFirst_ = preset.delayFirst;

    if (reverb_) {
        reverb_->setRoomSize(preset.reverbRoom);
        reverb_->setDamping(preset.reverbDamp);
        reverb_->setMix(preset.reverbMix);
    }
    reverbEnabled_ = preset.reverbEnabled;
}

// IR Loader
bool DSPEngine::loadIR(const std::string& path, std::string& error) {
    if (irLoader_) return irLoader_->loadIR(path, error);
    error = "IR Loader not initialized";
    return false;
}

void DSPEngine::setIREnabled(bool enabled) { irEnabled_ = enabled; }

void DSPEngine::setIRMix(float mix) {
    mix = std::clamp(mix, kMinMix, kMaxMix);
    if (irLoader_) irLoader_->setMix(mix);
}

void DSPEngine::setIRInputGain(float gainDb) {
    gainDb = std::clamp(gainDb, kMinGainDb, kMaxGainDb);
    if (irLoader_) irLoader_->setInputGain(gainDb);
}

void DSPEngine::setIROutputGain(float gainDb) {
    gainDb = std::clamp(gainDb, kMinGainDb, kMaxGainDb);
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

float DSPEngine::getLatencyMs() const {
    return latencyMonitor_.getLatencyMs();
}

float DSPEngine::getTheoreticalLatencyMs() const {
    return LatencyMonitor::calculateTheoreticalLatencyMs(blockSize_, sampleRate_);
}

} // namespace openamp
