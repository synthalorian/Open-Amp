#include "acoustic_sim.h"
#include "openamp/plugin_interface.h"

extern "C" {

openamp::AudioProcessor* createProcessor() {
    return new openamp::AcousticSimulator();
}

void destroyProcessor(openamp::AudioProcessor* processor) {
    delete processor;
}

const char* getPluginName() {
    return "Acoustic Sim";
}

const char* getPluginVersion() {
    return "1.0.0";
}

} // extern "C"
