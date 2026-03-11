//  OpenAmpObjC.h
//  Objective-C wrapper for C++ DSP classes

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface OFXAudioProcessor : NSObject

- (instancetype)initWithSampleRate:(double)sampleRate;

// Transport
- (BOOL)start;
- (void)stop;
- (BOOL)isRunning;

// Input/Output Gain
- (void)setInputGain:(float)gainDb;
- (void)setOutputGain:(float)gainDb;
- (float)inputGain;
- (float)outputGain;

// Amp Controls
- (void)setAmpEnabled:(BOOL)enabled;
- (void)setAmpGain:(float)gainDb;
- (void)setAmpDrive:(float)drive;
- (void)setAmpBass:(float)bassDb;
- (void)setAmpMid:(float)midDb;
- (void)setAmpTreble:(float)trebleDb;
- (void)setAmpPresence:(float)presenceDb;
- (void)setAmpMaster:(float)masterDb;

// Distortion
- (void)setDistortionEnabled:(BOOL)enabled;
- (void)setDistortionType:(int)type;
- (void)setDistortionDrive:(float)drive;
- (void)setDistortionTone:(float)tone;
- (void)setDistortionLevel:(float)level;

// Delay
- (void)setDelayEnabled:(BOOL)enabled;
- (void)setDelayTimeMs:(float)ms;
- (void)setDelayFeedback:(float)feedback;
- (void)setDelayMix:(float)mix;

// Reverb
- (void)setReverbEnabled:(BOOL)enabled;
- (void)setReverbRoomSize:(float)size;
- (void)setReverbDamping:(float)damping;
- (void)setReverbMix:(float)mix;

// Noise Gate
- (void)setNoiseGateEnabled:(BOOL)enabled;
- (void)setNoiseGateThreshold:(float)thresholdDb;
- (void)setNoiseGateAttack:(float)attackMs;
- (void)setNoiseGateHold:(float)holdMs;
- (void)setNoiseGateRelease:(float)releaseMs;
- (void)setNoiseGateRange:(float)rangeDb;

// Compressor
- (void)setCompressorEnabled:(BOOL)enabled;
- (void)setCompressorThreshold:(float)thresholdDb;
- (void)setCompressorRatio:(float)ratio;
- (void)setCompressorAttack:(float)attackMs;
- (void)setCompressorRelease:(float)releaseMs;
- (void)setCompressorMakeup:(float)makeupDb;

// EQ
- (void)setEQEnabled:(BOOL)enabled;
- (void)setEQBand:(int)band gain:(float)gainDb;
- (void)setEQLowCut:(float)freqHz;
- (void)setEQHighCut:(float)freqHz;

// Modulation
- (void)setModulationEnabled:(BOOL)enabled;
- (void)setModulationType:(int)type;
- (void)setModulationRate:(float)hz;
- (void)setModulationDepth:(float)depth;
- (void)setModulationMix:(float)mix;

// Wah
- (void)setWahEnabled:(BOOL)enabled;
- (void)setWahPosition:(float)position;
- (void)setWahMode:(int)mode;
- (void)setWahQ:(float)q;

// Presets
- (BOOL)savePresetToPath:(NSString *)path name:(NSString *)name;
- (BOOL)loadPresetFromPath:(NSString *)path;

// Meters
- (float)inputLevel;
- (float)outputLevel;
- (float)gainReduction;

@end

NS_ASSUME_NONNULL_END
