#include "distortion.h"
#include <memory>

namespace openamp {

class DistortionPlugin final : public PluginInterface {
public:
    AudioProcessorPtr createProcessor() override {
        return std::make_unique<Distortion>();
    }
    
    std::string getPluginName() const override { return "Distortion"; }
    std::string getPluginVersion() const override { return "1.0.0"; }
};

} // namespace openamp

GUITAR_AMP_PLUGIN_API openamp::PluginInterface* createPlugin() {
    return new openamp::DistortionPlugin();
}

GUITAR_AMP_PLUGIN_API void destroyPlugin(openamp::PluginInterface* plugin) {
    delete plugin;
}
