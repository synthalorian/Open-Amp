#include "noise_gate.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class NoiseGatePlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::NoiseGate>();
        }
        std::string getPluginName() const override { return "Noise Gate"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new NoiseGatePlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
