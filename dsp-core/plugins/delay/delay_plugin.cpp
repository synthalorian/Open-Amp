#include "delay.h"
#include <memory>

namespace openamp {

class DelayPlugin final : public PluginInterface {
public:
    AudioProcessorPtr createProcessor() override {
        return std::make_unique<Delay>();
    }
    
    std::string getPluginName() const override { return "Delay"; }
    std::string getPluginVersion() const override { return "1.0.0"; }
};

} // namespace openamp

GUITAR_AMP_PLUGIN_API openamp::PluginInterface* createPlugin() {
    return new openamp::DelayPlugin();
}

GUITAR_AMP_PLUGIN_API void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}
