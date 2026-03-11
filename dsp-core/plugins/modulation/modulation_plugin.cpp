#include "modulation.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class ModulationPlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::Modulation>();
        }
        std::string getPluginName() const override { return "Modulation"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new ModulationPlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
