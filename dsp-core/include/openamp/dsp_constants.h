#pragma once

#include <cstdint>

namespace openamp::constants {

// ---- Sample Rate & Buffer ----
constexpr double kMinSampleRate = 8000.0;
constexpr double kMaxSampleRate = 192000.0;
constexpr double kDefaultSampleRate = 48000.0;
constexpr uint32_t kMinBlockSize = 16;
constexpr uint32_t kMaxBlockSize = 4096;
constexpr uint32_t kDefaultBlockSize = 256;

// ---- Math ----
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

// ---- Amp Simulator ----
constexpr float kPresenceBaseHz = 5000.0f;
constexpr float kPresenceScalePerDb = 50.0f;
constexpr float kCabHighPassHz = 80.0f;
constexpr float kCabLowPassHz = 6000.0f;
constexpr float kPreEmphasisHz = 2000.0f;
constexpr float kDeEmphasisHz = 2000.0f;
constexpr float kGainRangeDb = 40.0f;
constexpr float kGainOffsetDb = -20.0f;
constexpr float kMasterRangeDb = 20.0f;
constexpr float kMasterOffsetDb = -10.0f;
constexpr float kPowerAmpSoftClipDrive = 2.0f;
constexpr float kPowerAmpHardClipThreshold = 0.95f;
constexpr float kDriveScale = 9.0f;   // drive_ * kDriveScale + 1.0
constexpr float kDriveOffset = 1.0f;

// ---- Tone Stack ----
constexpr float kBassBaseHz = 100.0f;
constexpr float kBassScalePerDb = 2.0f;
constexpr float kMidBaseHz = 1000.0f;
constexpr float kMidScalePerDb = 10.0f;
constexpr float kTrebleBaseHz = 5000.0f;
constexpr float kTrebleScalePerDb = 50.0f;

// ---- IR Loader ----
constexpr float kIRDefaultHighCutHz = 16000.0f;
constexpr float kIRDefaultLowCutHz = 80.0f;
constexpr float kIRMinHighCutHz = 1000.0f;
constexpr float kIRMaxHighCutHz = 20000.0f;
constexpr float kIRMinLowCutHz = 20.0f;
constexpr float kIRMaxLowCutHz = 500.0f;
constexpr float kIRNormalizePeak = 0.989f;       // -0.1 dB
constexpr float kIRSilenceThreshold = 0.001f;    // -60 dB

// ---- Noise Gate Defaults ----
constexpr float kDefaultNoiseGateThreshold = -40.0f;
constexpr float kDefaultNoiseGateAttack = 1.0f;
constexpr float kDefaultNoiseGateRelease = 100.0f;

// ---- Compressor Defaults ----
constexpr float kDefaultCompressorThreshold = -20.0f;
constexpr float kDefaultCompressorRatio = 4.0f;
constexpr float kDefaultCompressorAttack = 10.0f;
constexpr float kDefaultCompressorRelease = 100.0f;
constexpr float kDefaultCompressorMakeupGain = 3.0f;

// ---- Parameter Ranges (for clamping) ----
constexpr float kMinGainDb = -20.0f;
constexpr float kMaxGainDb = 20.0f;
constexpr float kMinDrive = 0.0f;
constexpr float kMaxDrive = 1.0f;
constexpr float kMinEQDb = -12.0f;
constexpr float kMaxEQDb = 12.0f;
constexpr float kMinMasterDb = -20.0f;
constexpr float kMaxMasterDb = 20.0f;
constexpr float kMinMix = 0.0f;
constexpr float kMaxMix = 1.0f;
constexpr float kMinDelayMs = 1.0f;
constexpr float kMaxDelayMs = 2000.0f;
constexpr float kMinFeedback = 0.0f;
constexpr float kMaxFeedback = 0.95f;
constexpr float kMinRoomSize = 0.0f;
constexpr float kMaxRoomSize = 1.0f;
constexpr float kMinDamping = 0.0f;
constexpr float kMaxDamping = 1.0f;
constexpr float kMinNoiseGateThreshold = -80.0f;
constexpr float kMaxNoiseGateThreshold = 0.0f;
constexpr float kMinAttackMs = 0.1f;
constexpr float kMaxAttackMs = 100.0f;
constexpr float kMinReleaseMs = 1.0f;
constexpr float kMaxReleaseMs = 1000.0f;
constexpr float kMinCompressorRatio = 1.0f;
constexpr float kMaxCompressorRatio = 20.0f;

} // namespace openamp::constants
