#include "openamp/plugin_loader.h"
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace openamp {

LoadedPlugin::LoadedPlugin(void* handle, PluginInterface* plugin, DestroyPluginFunc destroyFunc)
    : handle_(handle), plugin_(plugin), destroyFunc_(destroyFunc) {}

LoadedPlugin::~LoadedPlugin() {
    if (destroyFunc_ && plugin_) {
        destroyFunc_(plugin_);
    }

#if defined(_WIN32)
    if (handle_) {
        FreeLibrary(static_cast<HMODULE>(handle_));
    }
#else
    if (handle_) {
        dlclose(handle_);
    }
#endif
}

LoadedPlugin::LoadedPlugin(LoadedPlugin&& other) noexcept
    : handle_(other.handle_), plugin_(other.plugin_), destroyFunc_(other.destroyFunc_) {
    other.handle_ = nullptr;
    other.plugin_ = nullptr;
    other.destroyFunc_ = nullptr;
}

LoadedPlugin& LoadedPlugin::operator=(LoadedPlugin&& other) noexcept {
    if (this != &other) {
        this->~LoadedPlugin();
        handle_ = other.handle_;
        plugin_ = other.plugin_;
        destroyFunc_ = other.destroyFunc_;
        other.handle_ = nullptr;
        other.plugin_ = nullptr;
        other.destroyFunc_ = nullptr;
    }
    return *this;
}

std::unique_ptr<LoadedPlugin> PluginLoader::loadFromPath(const std::string& path, std::string& error) {
    error.clear();

#if defined(_WIN32)
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) {
        error = "Failed to load plugin library.";
        return nullptr;
    }

    auto createFunc = reinterpret_cast<CreatePluginFunc>(GetProcAddress(handle, "createPlugin"));
    auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(GetProcAddress(handle, "destroyPlugin"));
#else
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        const char* err = dlerror();
        error = err ? err : "Failed to load plugin library.";
        return nullptr;
    }

    auto createFunc = reinterpret_cast<CreatePluginFunc>(dlsym(handle, "createPlugin"));
    auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(dlsym(handle, "destroyPlugin"));
#endif

    if (!createFunc || !destroyFunc) {
        error = "Plugin entry points not found.";
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return nullptr;
    }

    PluginInterface* plugin = createFunc();
    if (!plugin) {
        error = "Plugin creation failed.";
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return nullptr;
    }

    return std::make_unique<LoadedPlugin>(handle, plugin, destroyFunc);
}

} // namespace openamp
