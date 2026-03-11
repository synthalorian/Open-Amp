//  GuitarAmpObjC.mm
//  Objective-C++ implementation wrapping C++ DSP core

#import "GuitarAmpObjC.h"
#include "guitar_amp/amp_simulator.h"
#include "guitar_amp/effect_chain.h"
#include "guitar_amp/input_processor.h"
#include "guitar_amp/preset_store.h"
#include "plugins/distortion/distortion.h"
#include "plugins/delay/delay.h"
#include "plugins/reverb/reverb.h"
#include "plugins/noise_gate/noise_gate.h"
#include "plugins/compressor/compressor.h"
#include "plugins/eq/eq.h"
#include "plugins/modulation/modulation.h"
#include "plugins/wah/wah.h"

#include <memory>

@interface GAAudioProcessor() {
    std::unique_ptr<guitar_amp::InputProcessor> processor_;
    std::unique_ptr<guitar_amp::AmpSimulator> amp_;
    std::unique_ptr<guitar_amp::EffectChain> effectChain_;
    std::unique_ptr<guitar_amp::Distortion> distortion_;
    std::unique_ptr<guitar_amp::Delay> delay_;
    std::unique_ptr<guitar_amp::Reverb> reverb_;
    std::unique_ptr<guitar_amp::NoiseGate> noiseGate_;
    std::unique_ptr<guitar_amp::Compressor> compressor_;
    std::unique_ptr<guitar_amp::EQ> eq_;
    std::unique_ptr<guitar_amp::Modulation> modulation_;
    std::unique_ptr<guitar_amp::Wah> wah_;
    
    double sampleRate_;
    bool isRunning_;
}

@end

@implementation GAAudioProcessor

- (instancetype)initWithSampleRate:(double)sampleRate {
    self = [super init];
    if (self) {
        sampleRate_ = sampleRate;
        isRunning_ = false;
        
        // Create DSP components
        amp_ = std::make_unique<guitar_amp::AmpSimulator>();
        amp_->prepare(sampleRate_, 1024);
        
        effectChain_ = std::make_unique<guitar_amp::EffectChain>();
        effectChain_->prepare(sampleRate_, 1024);
        
        distortion_ = std::make_unique<guitar_amp::Distortion>();
        distortion_->prepare(sampleRate_, 1024);
        
        delay_ = std::make_unique<guitar_amp::Delay>();
        delay_->prepare(sampleRate_, 1024);
        
        reverb_ = std::make_unique<guitar_amp::Reverb>();
        reverb_->prepare(sampleRate_, 1024);
        
        noiseGate_ = std::make_unique<guitar_amp::NoiseGate>();
        noiseGate_->prepare(sampleRate_, 1024);
        
        compressor_ = std::make_unique<guitar_amp::Compressor>();
        compressor_->prepare(sampleRate_, 1024);
        
        eq_ = std::make_unique<guitar_amp::EQ>();
        eq_->prepare(sampleRate_, 1024);
        
        modulation_ = std::make_unique<guitar_amp::Modulation>();
        modulation_->prepare(sampleRate_, 1024);
        
        wah_ = std::make_unique<guitar_amp::Wah>();
        wah_->prepare(sampleRate_, 1024);
    }
    return self;
}

- (BOOL)start {
    isRunning_ = true;
    return YES;
}

- (void)stop {
    isRunning_ = false;
}

- (BOOL)isRunning {
    return isRunning_;
}

#pragma mark - Input/Output Gain

- (void)setInputGain:(float)gainDb {
    if (processor_) processor_->setInputGain(gainDb);
}

- (void)setOutputGain:(float)gainDb {
    if (processor_) processor_->setOutputGain(gainDb);
}

- (float)inputGain {
    return processor_ ? processor_->getInputGain() : 0.0f;
}

- (float)outputGain {
    return processor_ ? processor_->getOutputGain() : 0.0f;
}

#pragma mark - Amp Controls

- (void)setAmpEnabled:(BOOL)enabled {
    // Toggle amp in signal chain
}

- (void)setAmpGain:(float)gainDb {
    amp_->setGain(gainDb);
}

- (void)setAmpDrive:(float)drive {
    amp_->setDrive(drive);
}

- (void)setAmpBass:(float)bassDb {
    amp_->setBass(bassDb);
}

- (void)setAmpMid:(float)midDb {
    amp_->setMid(midDb);
}

- (void)setAmpTreble:(float)trebleDb {
    amp_->setTreble(trebleDb);
}

- (void)setAmpPresence:(float)presenceDb {
    amp_->setPresence(presenceDb);
}

- (void)setAmpMaster:(float)masterDb {
    amp_->setMaster(masterDb);
}

#pragma mark - Distortion

- (void)setDistortionEnabled:(BOOL)enabled {
    distortion_->reset();
}

- (void)setDistortionType:(int)type {
    distortion_->setType(static_cast<guitar_amp::Distortion::Type>(type));
}

- (void)setDistortionDrive:(float)drive {
    distortion_->setDrive(drive);
}

- (void)setDistortionTone:(float)tone {
    distortion_->setTone(tone);
}

- (void)setDistortionLevel:(float)level {
    distortion_->setLevel(level);
}

#pragma mark - Delay

- (void)setDelayEnabled:(BOOL)enabled {
    delay_->reset();
}

- (void)setDelayTimeMs:(float)ms {
    delay_->setTimeMs(ms);
}

- (void)setDelayFeedback:(float)feedback {
    delay_->setFeedback(feedback);
}

- (void)setDelayMix:(float)mix {
    delay_->setMix(mix);
}

#pragma mark - Reverb

- (void)setReverbEnabled:(BOOL)enabled {
    reverb_->reset();
}

- (void)setReverbRoomSize:(float)size {
    reverb_->setRoomSize(size);
}

- (void)setReverbDamping:(float)damping {
    reverb_->setDamping(damping);
}

- (void)setReverbMix:(float)mix {
    reverb_->setMix(mix);
}

#pragma mark - Noise Gate

- (void)setNoiseGateEnabled:(BOOL)enabled {
    noiseGate_->reset();
}

- (void)setNoiseGateThreshold:(float)thresholdDb {
    noiseGate_->setThreshold(thresholdDb);
}

- (void)setNoiseGateAttack:(float)attackMs {
    noiseGate_->setAttack(attackMs);
}

- (void)setNoiseGateHold:(float)holdMs {
    noiseGate_->setHold(holdMs);
}

- (void)setNoiseGateRelease:(float)releaseMs {
    noiseGate_->setRelease(releaseMs);
}

- (void)setNoiseGateRange:(float)rangeDb {
    noiseGate_->setRange(rangeDb);
}

#pragma mark - Compressor

- (void)setCompressorEnabled:(BOOL)enabled {
    compressor_->reset();
}

- (void)setCompressorThreshold:(float)thresholdDb {
    compressor_->setThreshold(thresholdDb);
}

- (void)setCompressorRatio:(float)ratio {
    compressor_->setRatio(ratio);
}

- (void)setCompressorAttack:(float)attackMs {
    compressor_->setAttack(attackMs);
}

- (void)setCompressorRelease:(float)releaseMs {
    compressor_->setRelease(releaseMs);
}

- (void)setCompressorMakeup:(float)makeupDb {
    compressor_->setMakeupGain(makeupDb);
}

#pragma mark - EQ

- (void)setEQEnabled:(BOOL)enabled {
    eq_->reset();
}

- (void)setEQBand:(int)band gain:(float)gainDb {
    eq_->setBandGain(band, gainDb);
}

- (void)setEQLowCut:(float)freqHz {
    eq_->setLowCut(freqHz);
}

- (void)setEQHighCut:(float)freqHz {
    eq_->setHighCut(freqHz);
}

#pragma mark - Modulation

- (void)setModulationEnabled:(BOOL)enabled {
    modulation_->reset();
}

- (void)setModulationType:(int)type {
    modulation_->setType(static_cast<guitar_amp::Modulation::Type>(type));
}

- (void)setModulationRate:(float)hz {
    modulation_->setRate(hz);
}

- (void)setModulationDepth:(float)depth {
    modulation_->setDepth(depth);
}

- (void)setModulationMix:(float)mix {
    modulation_->setMix(mix);
}

#pragma mark - Wah

- (void)setWahEnabled:(BOOL)enabled {
    wah_->reset();
}

- (void)setWahPosition:(float)position {
    wah_->setPosition(position);
}

- (void)setWahMode:(int)mode {
    wah_->setMode(static_cast<guitar_amp::Wah::Mode>(mode));
}

- (void)setWahQ:(float)q {
    wah_->setQ(q);
}

#pragma mark - Presets

- (BOOL)savePresetToPath:(NSString *)path name:(NSString *)name {
    std::string cppPath = [path UTF8String];
    std::string cppName = [name UTF8String];
    // Implement preset saving
    return YES;
}

- (BOOL)loadPresetFromPath:(NSString *)path {
    std::string cppPath = [path UTF8String];
    // Implement preset loading
    return YES;
}

#pragma mark - Meters

- (float)inputLevel {
    return processor_ ? processor_->getInputLevel() : 0.0f;
}

- (float)outputLevel {
    return processor_ ? processor_->getOutputLevel() : 0.0f;
}

- (float)gainReduction {
    return compressor_->getGainReduction();
}

@end
