#include "wah.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class WahPlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::Wah>();
        }
        std::string getPluginName() const override { return "Wah"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new WahPlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
