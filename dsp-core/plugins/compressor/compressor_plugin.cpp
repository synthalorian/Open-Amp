#include "compressor.h"

extern "C" {

openamp::PluginInterface* createPlugin() {
    class CompressorPlugin : public openamp::PluginInterface {
    public:
        openamp::AudioProcessorPtr createProcessor() override {
            return std::make_unique<openamp::Compressor>();
        }
        std::string getPluginName() const override { return "Compressor"; }
        std::string getPluginVersion() const override { return "1.0.0"; }
    };
    return new CompressorPlugin();
}

void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}

}
