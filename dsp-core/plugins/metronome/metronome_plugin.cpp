#include "metronome.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class MetronomePlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::Metronome>();
        }
        std::string getPluginName() const override { return "Metronome"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new MetronomePlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
