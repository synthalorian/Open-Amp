#include "DSPCore.h"
#include "openamp/dsp_engine.h"
#include <cstring>
#include <memory>

DSPEngineRef dsp_engine_create(double sampleRate, uint32_t blockSize) {
    try {
        auto* engine = new openamp::DSPEngine();
        engine->prepare(sampleRate, blockSize);
        return reinterpret_cast<DSPEngineRef>(engine);
    } catch (...) {
        return nullptr;
    }
}

void dsp_engine_destroy(DSPEngineRef engine) {
    delete reinterpret_cast<openamp::DSPEngine*>(engine);
}

void dsp_engine_process(DSPEngineRef engine, float* input, float* output, uint32_t numFrames) {
    if (!engine || !input) return;

    auto* dsp = reinterpret_cast<openamp::DSPEngine*>(engine);

    // Copy input to output for in-place processing
    if (output && output != input) {
        memcpy(output, input, numFrames * sizeof(float));
    }

    float* data = output ? output : input;
    dsp->process(data, numFrames);
}

void dsp_engine_set_gain(DSPEngineRef engine, float gainDb) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setGain(gainDb);
}

void dsp_engine_set_drive(DSPEngineRef engine, float drive) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDrive(drive);
}

void dsp_engine_set_bass(DSPEngineRef engine, float db) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setBass(db);
}

void dsp_engine_set_mid(DSPEngineRef engine, float db) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setMid(db);
}

void dsp_engine_set_treble(DSPEngineRef engine, float db) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setTreble(db);
}

void dsp_engine_set_presence(DSPEngineRef engine, float db) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setPresence(db);
}

void dsp_engine_set_master(DSPEngineRef engine, float db) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setMaster(db);
}

void dsp_engine_set_amp_enabled(DSPEngineRef engine, int enabled) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setAmpEnabled(enabled != 0);
}

void dsp_engine_set_distortion_enabled(DSPEngineRef engine, int enabled) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDistortionEnabled(enabled != 0);
}

void dsp_engine_set_delay_enabled(DSPEngineRef engine, int enabled) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDelayEnabled(enabled != 0);
}

void dsp_engine_set_reverb_enabled(DSPEngineRef engine, int enabled) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setReverbEnabled(enabled != 0);
}

void dsp_engine_set_distortion_type(DSPEngineRef engine, int type) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDistortionType(type);
}

void dsp_engine_set_distortion_drive(DSPEngineRef engine, float drive) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDistortionDrive(drive);
}

void dsp_engine_set_distortion_tone(DSPEngineRef engine, float tone) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDistortionTone(tone);
}

void dsp_engine_set_distortion_level(DSPEngineRef engine, float level) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDistortionLevel(level);
}

void dsp_engine_set_delay_time(DSPEngineRef engine, float ms) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDelayTime(ms);
}

void dsp_engine_set_delay_feedback(DSPEngineRef engine, float feedback) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDelayFeedback(feedback);
}

void dsp_engine_set_delay_mix(DSPEngineRef engine, float mix) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDelayMix(mix);
}

void dsp_engine_set_delay_first(DSPEngineRef engine, int first) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setDelayFirst(first != 0);
}

void dsp_engine_set_reverb_room(DSPEngineRef engine, float room) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setReverbRoom(room);
}

void dsp_engine_set_reverb_damp(DSPEngineRef engine, float damp) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setReverbDamp(damp);
}

void dsp_engine_set_reverb_mix(DSPEngineRef engine, float mix) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->setReverbMix(mix);
}

int dsp_engine_load_preset(DSPEngineRef engine, const char* path, char* error, size_t errorSize) {
    if (!engine || !path) {
        if (error && errorSize > 0) strncpy(error, "Invalid parameters", errorSize);
        return 0;
    }

    auto* dsp = reinterpret_cast<openamp::DSPEngine*>(engine);
    std::string err;
    if (dsp->loadPreset(path, err)) {
        return 1;
    }

    if (error && errorSize > 0) {
        strncpy(error, err.c_str(), errorSize - 1);
        error[errorSize - 1] = '\0';
    }
    return 0;
}

int dsp_engine_save_preset(DSPEngineRef engine, const char* path, char* error, size_t errorSize) {
    if (!engine || !path) {
        if (error && errorSize > 0) strncpy(error, "Invalid parameters", errorSize);
        return 0;
    }

    auto* dsp = reinterpret_cast<openamp::DSPEngine*>(engine);
    std::string err;
    if (dsp->savePreset(path, err)) {
        return 1;
    }

    if (error && errorSize > 0) {
        strncpy(error, err.c_str(), errorSize - 1);
        error[errorSize - 1] = '\0';
    }
    return 0;
}

void dsp_engine_get_preset(DSPEngineRef engine, DSPPreset* preset) {
    if (!engine || !preset) return;

    auto* dsp = reinterpret_cast<openamp::DSPEngine*>(engine);
    const auto& src = dsp->getCurrentPreset();

    strncpy(preset->name, src.name.c_str(), 63);
    preset->name[63] = '\0';

    preset->inputGainDb = src.inputGainDb;
    preset->outputGainDb = src.outputGainDb;
    preset->ampEnabled = src.ampEnabled ? 1 : 0;
    preset->effectsEnabled = src.effectsEnabled ? 1 : 0;
    preset->delayEnabled = src.delayEnabled ? 1 : 0;
    preset->reverbEnabled = src.reverbEnabled ? 1 : 0;
    preset->distortionEnabled = src.distortionEnabled ? 1 : 0;
    preset->delayFirst = src.delayFirst ? 1 : 0;
    preset->delayTimeMs = src.delayTimeMs;
    preset->delayFeedback = src.delayFeedback;
    preset->delayMix = src.delayMix;
    preset->reverbRoom = src.reverbRoom;
    preset->reverbDamp = src.reverbDamp;
    preset->reverbMix = src.reverbMix;
    preset->distortionDrive = src.distortionDrive;
    preset->distortionTone = src.distortionTone;
    preset->distortionLevel = src.distortionLevel;
    preset->distortionType = src.distortionType;
    preset->ampGainDb = src.ampGainDb;
    preset->ampDrive = src.ampDrive;
    preset->ampBassDb = src.ampBassDb;
    preset->ampMidDb = src.ampMidDb;
    preset->ampTrebleDb = src.ampTrebleDb;
    preset->ampPresenceDb = src.ampPresenceDb;
    preset->ampMasterDb = src.ampMasterDb;
}

void dsp_engine_apply_preset(DSPEngineRef engine, const DSPPreset* preset) {
    if (!engine || !preset) return;

    auto* dsp = reinterpret_cast<openamp::DSPEngine*>(engine);

    openamp::Preset p;
    p.name = preset->name;
    p.inputGainDb = preset->inputGainDb;
    p.outputGainDb = preset->outputGainDb;
    p.ampEnabled = preset->ampEnabled != 0;
    p.effectsEnabled = preset->effectsEnabled != 0;
    p.delayEnabled = preset->delayEnabled != 0;
    p.reverbEnabled = preset->reverbEnabled != 0;
    p.distortionEnabled = preset->distortionEnabled != 0;
    p.delayFirst = preset->delayFirst != 0;
    p.delayTimeMs = preset->delayTimeMs;
    p.delayFeedback = preset->delayFeedback;
    p.delayMix = preset->delayMix;
    p.reverbRoom = preset->reverbRoom;
    p.reverbDamp = preset->reverbDamp;
    p.reverbMix = preset->reverbMix;
    p.distortionDrive = preset->distortionDrive;
    p.distortionTone = preset->distortionTone;
    p.distortionLevel = preset->distortionLevel;
    p.distortionType = preset->distortionType;
    p.ampGainDb = preset->ampGainDb;
    p.ampDrive = preset->ampDrive;
    p.ampBassDb = preset->ampBassDb;
    p.ampMidDb = preset->ampMidDb;
    p.ampTrebleDb = preset->ampTrebleDb;
    p.ampPresenceDb = preset->ampPresenceDb;
    p.ampMasterDb = preset->ampMasterDb;

    dsp->applyPreset(p);
}

void dsp_engine_reset(DSPEngineRef engine) {
    if (engine) reinterpret_cast<openamp::DSPEngine*>(engine)->reset();
}

double dsp_engine_get_latency(DSPEngineRef engine) {
    if (!engine) return 0.0;
    auto* dsp = reinterpret_cast<openamp::DSPEngine*>(engine);
    return static_cast<double>(dsp->getBlockSize()) / dsp->getSampleRate();
}
