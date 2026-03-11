#include "looper.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class LooperPlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::Looper>();
        }
        std::string getPluginName() const override { return "Looper"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new LooperPlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
