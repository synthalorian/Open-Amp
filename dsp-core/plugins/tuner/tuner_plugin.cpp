#include "tuner.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class TunerPlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::Tuner>();
        }
        std::string getPluginName() const override { return "Tuner"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new TunerPlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
