#pragma once

// DSPCore - Swift-accessible wrapper for openamp DSP engine

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to the DSP engine
typedef struct DSPEngine* DSPEngineRef;

// Engine lifecycle
DSPEngineRef dsp_engine_create(double sampleRate, uint32_t blockSize);
void dsp_engine_destroy(DSPEngineRef engine);

// Processing
void dsp_engine_process(DSPEngineRef engine, float* input, float* output, uint32_t numFrames);

// Amp controls
void dsp_engine_set_gain(DSPEngineRef engine, float gainDb);
void dsp_engine_set_drive(DSPEngineRef engine, float drive);
void dsp_engine_set_bass(DSPEngineRef engine, float db);
void dsp_engine_set_mid(DSPEngineRef engine, float db);
void dsp_engine_set_treble(DSPEngineRef engine, float db);
void dsp_engine_set_presence(DSPEngineRef engine, float db);
void dsp_engine_set_master(DSPEngineRef engine, float db);

// Effect toggles
void dsp_engine_set_amp_enabled(DSPEngineRef engine, int enabled);
void dsp_engine_set_distortion_enabled(DSPEngineRef engine, int enabled);
void dsp_engine_set_delay_enabled(DSPEngineRef engine, int enabled);
void dsp_engine_set_reverb_enabled(DSPEngineRef engine, int enabled);

// Distortion
void dsp_engine_set_distortion_type(DSPEngineRef engine, int type);
void dsp_engine_set_distortion_drive(DSPEngineRef engine, float drive);
void dsp_engine_set_distortion_tone(DSPEngineRef engine, float tone);
void dsp_engine_set_distortion_level(DSPEngineRef engine, float level);

// Delay
void dsp_engine_set_delay_time(DSPEngineRef engine, float ms);
void dsp_engine_set_delay_feedback(DSPEngineRef engine, float feedback);
void dsp_engine_set_delay_mix(DSPEngineRef engine, float mix);
void dsp_engine_set_delay_first(DSPEngineRef engine, int first);

// Reverb
void dsp_engine_set_reverb_room(DSPEngineRef engine, float room);
void dsp_engine_set_reverb_damp(DSPEngineRef engine, float damp);
void dsp_engine_set_reverb_mix(DSPEngineRef engine, float mix);

// Presets
typedef struct {
    char name[64];
    float inputGainDb;
    float outputGainDb;
    int ampEnabled;
    int effectsEnabled;
    int delayEnabled;
    int reverbEnabled;
    int distortionEnabled;
    int delayFirst;
    float delayTimeMs;
    float delayFeedback;
    float delayMix;
    float reverbRoom;
    float reverbDamp;
    float reverbMix;
    float distortionDrive;
    float distortionTone;
    float distortionLevel;
    int distortionType;
    float ampGainDb;
    float ampDrive;
    float ampBassDb;
    float ampMidDb;
    float ampTrebleDb;
    float ampPresenceDb;
    float ampMasterDb;
} DSPPreset;

int dsp_engine_load_preset(DSPEngineRef engine, const char* path, char* error, size_t errorSize);
int dsp_engine_save_preset(DSPEngineRef engine, const char* path, char* error, size_t errorSize);
void dsp_engine_get_preset(DSPEngineRef engine, DSPPreset* preset);
void dsp_engine_apply_preset(DSPEngineRef engine, const DSPPreset* preset);

// Utility
void dsp_engine_reset(DSPEngineRef engine);
double dsp_engine_get_latency(DSPEngineRef engine);

#ifdef __cplusplus
}
#endif
