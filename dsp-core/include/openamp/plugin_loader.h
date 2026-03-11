#pragma once

#include "openamp/plugin_interface.h"
#include <memory>
#include <string>

namespace openamp {

class LoadedPlugin {
public:
    LoadedPlugin(void* handle, PluginInterface* plugin, DestroyPluginFunc destroyFunc);
    ~LoadedPlugin();
    
    LoadedPlugin(const LoadedPlugin&) = delete;
    LoadedPlugin& operator=(const LoadedPlugin&) = delete;
    
    LoadedPlugin(LoadedPlugin&& other) noexcept;
    LoadedPlugin& operator=(LoadedPlugin&& other) noexcept;
    
    PluginInterface* get() const { return plugin_; }
    void* getHandle() const { return handle_; }
    
private:
    void* handle_ = nullptr;
    PluginInterface* plugin_ = nullptr;
    DestroyPluginFunc destroyFunc_ = nullptr;
};

class PluginLoader {
public:
    static std::unique_ptr<LoadedPlugin> loadFromPath(const std::string& path, std::string& error);
};

} // namespace openamp
