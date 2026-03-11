// OpenAmpBridgingHeader.h
// Bridging header for C++ DSP core

#ifndef OpenAmpBridgingHeader_h
#define OpenAmpBridgingHeader_h

// Include C headers that need to be accessible from Swift
// The actual DSP processing will be wrapped in Objective-C++ classes

#import "amp_simulator.h"
#import "effect_chain.h"
#import "plugin_interface.h"
#import "preset_store.h"
#import "input_processor.h"
#import "dsp_utils.h"

#endif /* OpenAmpBridgingHeader_h */
