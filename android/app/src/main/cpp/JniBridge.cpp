#include "AudioEngine.h"
#include <jni.h>
#include <memory>
#include <string>
#include <android/log.h>

#define LOG_TAG "OpenAmp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static std::unique_ptr<openamp_android::AudioEngine> g_engine;

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeCreate(JNIEnv* env, jobject obj) {
    try {
        if (g_engine) {
            LOGW("JniBridge: nativeCreate called but engine already exists");
            return;
        }
        LOGI("JniBridge: Creating AudioEngine...");
        g_engine = std::make_unique<openamp_android::AudioEngine>();
        if (!g_engine) {
            LOGE("JniBridge: Failed to create AudioEngine - make_unique returned null");
        } else {
            LOGI("JniBridge: AudioEngine created successfully");
        }
    } catch (const std::exception& e) {
        LOGE("JniBridge: Exception in nativeCreate: %s", e.what());
    } catch (...) {
        LOGE("JniBridge: Unknown exception in nativeCreate");
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeStart(JNIEnv* env, jobject obj) {
    LOGI("JniBridge: nativeStart called");
    
    if (!g_engine) {
        LOGE("JniBridge: nativeStart - g_engine is null!");
        return JNI_FALSE;
    }
    
    try {
        LOGI("JniBridge: About to call g_engine->start()");
        bool result = g_engine->start();
        LOGI("JniBridge: g_engine->start() returned %s", result ? "true" : "false");
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("JniBridge: std::exception in nativeStart: %s", e.what());
        return JNI_FALSE;
    } catch (...) {
        LOGE("JniBridge: Unknown exception in nativeStart");
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeStop(JNIEnv*, jobject) {
    if (g_engine) g_engine->stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeRelease(JNIEnv*, jobject) {
    g_engine.reset();
}

// Gain controls
extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetInputGain(JNIEnv*, jobject, jfloat db) {
    if (g_engine) g_engine->setInputGain(db);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetOutputGain(JNIEnv*, jobject, jfloat db) {
    if (g_engine) g_engine->setOutputGain(db);
}

// Amp controls
extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetAmpEnabled(JNIEnv*, jobject, jboolean enabled) {
    if (g_engine) g_engine->setAmpEnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetAmpGainDb(JNIEnv*, jobject, jfloat db) {
    if (g_engine) g_engine->setAmpGainDb(db);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetAmpDrive(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setAmpDrive(amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetAmpBassDb(JNIEnv*, jobject, jfloat db) {
    if (g_engine) g_engine->setAmpBassDb(db);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetAmpMidDb(JNIEnv*, jobject, jfloat db) {
    if (g_engine) g_engine->setAmpMidDb(db);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetAmpTrebleDb(JNIEnv*, jobject, jfloat db) {
    if (g_engine) g_engine->setAmpTrebleDb(db);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetAmpPresenceDb(JNIEnv*, jobject, jfloat db) {
    if (g_engine) g_engine->setAmpPresenceDb(db);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetAmpMasterDb(JNIEnv*, jobject, jfloat db) {
    if (g_engine) g_engine->setAmpMasterDb(db);
}

// Distortion controls
extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDistortionEnabled(JNIEnv*, jobject, jboolean enabled) {
    if (g_engine) g_engine->setDistortionEnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDistortionDrive(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setDistortionDrive(amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDistortionTone(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setDistortionTone(amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDistortionLevel(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setDistortionLevel(amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDistortionType(JNIEnv*, jobject, jint type) {
    if (g_engine) g_engine->setDistortionType(type);
}

// Delay controls
extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDelayTimeMs(JNIEnv*, jobject, jfloat ms) {
    if (g_engine) g_engine->setDelayTimeMs(ms);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDelayFeedback(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setDelayFeedback(amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDelayMix(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setDelayMix(amount);
}

// Reverb controls
extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetReverbRoomSize(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setReverbRoomSize(amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetReverbDamping(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setReverbDamping(amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetReverbMix(JNIEnv*, jobject, jfloat amount) {
    if (g_engine) g_engine->setReverbMix(amount);
}

// Device selection
extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetInputDeviceId(JNIEnv*, jobject, jint deviceId) {
    if (g_engine) g_engine->setInputDeviceId(deviceId);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetOutputDeviceId(JNIEnv*, jobject, jint deviceId) {
    if (g_engine) g_engine->setOutputDeviceId(deviceId);
}

// Effect toggles
extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDelayEnabled(JNIEnv*, jobject, jboolean enabled) {
    if (g_engine) g_engine->setDelayEnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetReverbEnabled(JNIEnv*, jobject, jboolean enabled) {
    if (g_engine) g_engine->setReverbEnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetDelayFirst(JNIEnv*, jobject, jboolean enabled) {
    if (g_engine) g_engine->setDelayFirst(enabled == JNI_TRUE);
}

// Presets
extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeSavePreset(JNIEnv* env, jobject, jstring path, jstring name) {
    if (!g_engine) return JNI_FALSE;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    const char* nameStr = env->GetStringUTFChars(name, nullptr);
    bool ok = g_engine->savePreset(std::string(pathStr), std::string(nameStr));
    env->ReleaseStringUTFChars(path, pathStr);
    env->ReleaseStringUTFChars(name, nameStr);
    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeLoadPreset(JNIEnv* env, jobject, jstring path) {
    if (!g_engine) return JNI_FALSE;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    bool ok = g_engine->loadPreset(std::string(pathStr));
    env->ReleaseStringUTFChars(path, pathStr);
    return ok ? JNI_TRUE : JNI_FALSE;
}

// Meters
extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetInputLevel(JNIEnv*, jobject) {
    return g_engine ? g_engine->getInputLevel() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetOutputLevel(JNIEnv*, jobject) {
    return g_engine ? g_engine->getOutputLevel() : 0.0f;
}

// Getters
extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetInputGainDb(JNIEnv*, jobject) {
    return g_engine ? g_engine->getInputGainDb() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetOutputGainDb(JNIEnv*, jobject) {
    return g_engine ? g_engine->getOutputGainDb() : 0.0f;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeGetAmpEnabled(JNIEnv*, jobject) {
    return g_engine && g_engine->getAmpEnabled() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeGetEffectsEnabled(JNIEnv*, jobject) {
    return g_engine && g_engine->getEffectsEnabled() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeGetDelayEnabled(JNIEnv*, jobject) {
    return g_engine && g_engine->getDelayEnabled() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeGetReverbEnabled(JNIEnv*, jobject) {
    return g_engine && g_engine->getReverbEnabled() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeGetDistortionEnabled(JNIEnv*, jobject) {
    return g_engine && g_engine->getDistortionEnabled() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeGetDelayFirst(JNIEnv*, jobject) {
    return g_engine && g_engine->getDelayFirst() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetDelayTimeMs(JNIEnv*, jobject) {
    return g_engine ? g_engine->getDelayTimeMs() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetDelayFeedback(JNIEnv*, jobject) {
    return g_engine ? g_engine->getDelayFeedback() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetDelayMix(JNIEnv*, jobject) {
    return g_engine ? g_engine->getDelayMix() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetReverbRoom(JNIEnv*, jobject) {
    return g_engine ? g_engine->getReverbRoom() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetReverbDamp(JNIEnv*, jobject) {
    return g_engine ? g_engine->getReverbDamp() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetReverbMix(JNIEnv*, jobject) {
    return g_engine ? g_engine->getReverbMix() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetDistortionDrive(JNIEnv*, jobject) {
    return g_engine ? g_engine->getDistortionDrive() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetDistortionTone(JNIEnv*, jobject) {
    return g_engine ? g_engine->getDistortionTone() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetDistortionLevel(JNIEnv*, jobject) {
    return g_engine ? g_engine->getDistortionLevel() : 0.0f;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_openamp_AudioEngine_nativeGetDistortionType(JNIEnv*, jobject) {
    return g_engine ? g_engine->getDistortionType() : 0;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetAmpGainDb(JNIEnv*, jobject) {
    return g_engine ? g_engine->getAmpGainDb() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetAmpDrive(JNIEnv*, jobject) {
    return g_engine ? g_engine->getAmpDrive() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetAmpBassDb(JNIEnv*, jobject) {
    return g_engine ? g_engine->getAmpBassDb() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetAmpMidDb(JNIEnv*, jobject) {
    return g_engine ? g_engine->getAmpMidDb() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetAmpTrebleDb(JNIEnv*, jobject) {
    return g_engine ? g_engine->getAmpTrebleDb() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetAmpPresenceDb(JNIEnv*, jobject) {
    return g_engine ? g_engine->getAmpPresenceDb() : 0.0f;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetAmpMasterDb(JNIEnv*, jobject) {
    return g_engine ? g_engine->getAmpMasterDb() : 0.0f;
}

// IR Loader (NEW)
extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeLoadIRFromWavFile(JNIEnv* env, jobject, jstring path) {
    if (!g_engine) return JNI_FALSE;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    bool ok = g_engine->loadIRFromWavFile(std::string(pathStr));
    env->ReleaseStringUTFChars(path, pathStr);
    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetIREnabled(JNIEnv*, jobject, jboolean enabled) {
    if (g_engine) g_engine->setIREnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetIRMix(JNIEnv*, jobject, jfloat mix) {
    if (g_engine) g_engine->setIRMix(mix);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetIRInputGain(JNIEnv*, jobject, jfloat gainDb) {
    if (g_engine) g_engine->setIRInputGain(gainDb);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetIROutputGain(JNIEnv*, jobject, jfloat gainDb) {
    if (g_engine) g_engine->setIROutputGain(gainDb);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetIRHighCut(JNIEnv*, jobject, jfloat hz) {
    if (g_engine) g_engine->setIRHighCut(hz);
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetIRLowCut(JNIEnv*, jobject, jfloat hz) {
    if (g_engine) g_engine->setIRLowCut(hz);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_openamp_AudioEngine_nativeGetIRName(JNIEnv* env, jobject) {
    std::string name = g_engine ? g_engine->getIRName() : "";
    return env->NewStringUTF(name.c_str());
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetIRCPUsage(JNIEnv*, jobject) {
    return g_engine ? g_engine->getIRCPUsage() : 0.0f;
}

// Latency monitoring (NEW)
extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetLatencyMs(JNIEnv*, jobject) {
    return g_engine ? g_engine->getLatencyMs() : 0.0f;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_openamp_AudioEngine_nativeGetBufferSize(JNIEnv*, jobject) {
    return g_engine ? static_cast<jint>(g_engine->getBufferSize()) : 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_openamp_AudioEngine_nativeGetSampleRate(JNIEnv*, jobject) {
    return g_engine ? static_cast<jint>(g_engine->getSampleRate()) : 0;
}

// Legacy cabinet IR (kept for compatibility)
extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeSetCabIRFromFile(JNIEnv* env, jobject, jstring path) {
    if (!g_engine) return JNI_FALSE;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    bool ok = g_engine->setCabIRFromFile(std::string(pathStr));
    env->ReleaseStringUTFChars(path, pathStr);
    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_openamp_AudioEngine_nativeGetCabIrPath(JNIEnv* env, jobject) {
    std::string path = g_engine ? g_engine->getCabIrPath() : "";
    return env->NewStringUTF(path.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeGetClipping(JNIEnv*, jobject) {
    return g_engine && g_engine->getClipping() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeResetClipIndicator(JNIEnv*, jobject) {
    if (g_engine) g_engine->resetClipIndicator();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_openamp_AudioEngine_nativeGetDebugStatus(JNIEnv* env, jobject) {
    std::string status = g_engine ? g_engine->getDebugStatus() : "running=0 no_engine=1";
    return env->NewStringUTF(status.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetInputChannelMode(JNIEnv*, jobject, jint mode) {
    if (g_engine) g_engine->setInputChannelMode(mode);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_openamp_AudioEngine_nativeGetInputChannelMode(JNIEnv*, jobject) {
    return g_engine ? g_engine->getInputChannelMode() : 2;
}

extern "C" JNIEXPORT void JNICALL
Java_com_openamp_AudioEngine_nativeSetTestToneEnabled(JNIEnv*, jobject, jboolean enabled) {
    if (g_engine) g_engine->setTestToneEnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_openamp_AudioEngine_nativeGetTestToneEnabled(JNIEnv*, jobject) {
    return g_engine && g_engine->getTestToneEnabled() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_openamp_AudioEngine_nativeGetRawInputLevel(JNIEnv*, jobject) {
    return g_engine ? g_engine->getRawInputLevel() : 0.0f;
}
