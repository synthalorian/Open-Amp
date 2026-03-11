#include "reverb.h"
#include <memory>

namespace openamp {

class ReverbPlugin final : public PluginInterface {
public:
    AudioProcessorPtr createProcessor() override {
        return std::make_unique<Reverb>();
    }
    
    std::string getPluginName() const override { return "Reverb"; }
    std::string getPluginVersion() const override { return "1.0.0"; }
};

} // namespace openamp

GUITAR_AMP_PLUGIN_API openamp::PluginInterface* createPlugin() {
    return new openamp::ReverbPlugin();
}

GUITAR_AMP_PLUGIN_API void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}
