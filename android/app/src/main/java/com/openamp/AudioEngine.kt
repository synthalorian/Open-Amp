package com.openamp

class AudioEngine {
    init {
        try {
            System.loadLibrary("openamp-native")
        } catch (e: UnsatisfiedLinkError) {
            throw RuntimeException("Failed to load native library: ${e.message}")
        }
    }

    external fun nativeCreate()
    external fun nativeStart(): Boolean
    external fun nativeStop()
    external fun nativeRelease()
    external fun nativeSetInputGain(db: Float)
    external fun nativeSetOutputGain(db: Float)
    external fun nativeSetAmpEnabled(enabled: Boolean)
    external fun nativeSetEffectsEnabled(enabled: Boolean)
    external fun nativeSetDelayTimeMs(ms: Float)
    external fun nativeSetDelayFeedback(amount: Float)
    external fun nativeSetDelayMix(amount: Float)
    external fun nativeSetReverbRoomSize(amount: Float)
    external fun nativeSetReverbDamping(amount: Float)
    external fun nativeSetReverbMix(amount: Float)
    external fun nativeSetInputDeviceId(deviceId: Int)
    external fun nativeSetOutputDeviceId(deviceId: Int)
    external fun nativeSetDelayEnabled(enabled: Boolean)
    external fun nativeSetReverbEnabled(enabled: Boolean)
    external fun nativeSetDistortionEnabled(enabled: Boolean)
    external fun nativeSetDelayFirst(enabled: Boolean)
    external fun nativeSavePreset(path: String, name: String): Boolean
    external fun nativeLoadPreset(path: String): Boolean
    external fun nativeGetInputLevel(): Float
    external fun nativeGetOutputLevel(): Float
    external fun nativeGetLatencyMs(): Float
    external fun nativeGetBufferSize(): Int
    external fun nativeGetSampleRate(): Int

    external fun nativeSetNoiseGateEnabled(enabled: Boolean)
    external fun nativeSetNoiseGateThreshold(db: Float)
    external fun nativeSetNoiseGateAttack(ms: Float)
    external fun nativeSetNoiseGateRelease(ms: Float)
    external fun nativeGetNoiseGateEnabled(): Boolean
    external fun nativeGetNoiseGateThreshold(): Float

    external fun nativeLoadIRFromWavFile(path: String): Boolean
    external fun nativeSetIREnabled(enabled: Boolean)
    external fun nativeSetIRMix(mix: Float)
    external fun nativeSetIRInputGain(db: Float)
    external fun nativeSetIROutputGain(db: Float)
    external fun nativeSetIRHighCut(hz: Float)
    external fun nativeSetIRLowCut(hz: Float)
    external fun nativeGetIRName(): String
    external fun nativeGetIRCPUsage(): Float
    external fun nativeGetInputGainDb(): Float
    external fun nativeGetOutputGainDb(): Float
    external fun nativeGetAmpEnabled(): Boolean
    external fun nativeGetEffectsEnabled(): Boolean
    external fun nativeGetDelayEnabled(): Boolean
    external fun nativeGetReverbEnabled(): Boolean
    external fun nativeGetDistortionEnabled(): Boolean
    external fun nativeGetDelayFirst(): Boolean
    external fun nativeGetDelayTimeMs(): Float
    external fun nativeGetDelayFeedback(): Float
    external fun nativeGetDelayMix(): Float
    external fun nativeGetReverbRoom(): Float
    external fun nativeGetReverbDamp(): Float
    external fun nativeGetReverbMix(): Float
    external fun nativeGetDistortionDrive(): Float
    external fun nativeGetDistortionTone(): Float
    external fun nativeGetDistortionLevel(): Float
    external fun nativeGetDistortionType(): Int
    external fun nativeSetAmpGainDb(db: Float)
    external fun nativeSetAmpDrive(amount: Float)
    external fun nativeSetAmpBassDb(db: Float)
    external fun nativeSetAmpMidDb(db: Float)
    external fun nativeSetAmpTrebleDb(db: Float)
    external fun nativeSetAmpPresenceDb(db: Float)
    external fun nativeSetAmpMasterDb(db: Float)
    external fun nativeSetDistortionDrive(amount: Float)
    external fun nativeSetDistortionTone(amount: Float)
    external fun nativeSetDistortionLevel(amount: Float)
    external fun nativeSetDistortionType(type: Int)
    external fun nativeGetAmpGainDb(): Float
    external fun nativeGetAmpDrive(): Float
    external fun nativeGetAmpBassDb(): Float
    external fun nativeGetAmpMidDb(): Float
    external fun nativeGetAmpTrebleDb(): Float
    external fun nativeGetAmpPresenceDb(): Float
    external fun nativeGetAmpMasterDb(): Float
    external fun nativeSetCabIRFromFile(path: String): Boolean
    external fun nativeGetCabIrPath(): String
    external fun nativeGetClipping(): Boolean
    external fun nativeResetClipIndicator()
    external fun nativeGetDebugStatus(): String
    external fun nativeSetInputChannelMode(mode: Int)
    external fun nativeGetInputChannelMode(): Int
    external fun nativeSetTestToneEnabled(enabled: Boolean)
    external fun nativeGetTestToneEnabled(): Boolean
    external fun nativeGetRawInputLevel(): Float

    // Looper
    external fun nativeLooperRecord()
    external fun nativeLooperPlay()
    external fun nativeLooperStop()
    external fun nativeLooperClear()
    external fun nativeLooperUndo()
    external fun nativeLooperSetMix(mix: Float)
    external fun nativeLooperGetState(): Int
    external fun nativeLooperGetPosition(): Float
    external fun nativeLooperGetDuration(): Float

    // Metronome
    external fun nativeMetronomeStart()
    external fun nativeMetronomeStop()
    external fun nativeMetronomeSetTempo(bpm: Float)
    external fun nativeMetronomeSetVolume(volume: Float)
    external fun nativeMetronomeGetTempo(): Float
    external fun nativeMetronomeIsPlaying(): Boolean
}
