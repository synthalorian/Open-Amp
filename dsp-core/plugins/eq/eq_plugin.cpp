#include "eq.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class EQPlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::EQ>();
        }
        std::string getPluginName() const override { return "EQ"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new EQPlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
