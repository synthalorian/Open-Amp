#include "cabinet.h"
#include "openamp/plugin_interface.h"

extern "C" {

openamp::AudioProcessor* createProcessor() {
    return new openamp::CabinetSimulator();
}

void destroyProcessor(openamp::AudioProcessor* processor) {
    delete processor;
}

const char* getPluginName() {
    return "Cabinet";
}

const char* getPluginVersion() {
    return "1.0.0";
}

} // extern "C"
