#include "recorder.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class RecorderPlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::Recorder>();
        }
        std::string getPluginName() const override { return "Recorder"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new RecorderPlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
