#include "audio_backend.h"
#include "pipewire_backend.h"
#include "alsa_backend.h"

namespace openamp {

std::unique_ptr<AudioBackend> createBestBackend() {
#ifdef USE_PIPEWIRE
    auto pw = createPipeWireBackend();
    if (pw && pw->isAvailable()) {
        return pw;
    }
#endif

#ifdef USE_ALSA
    auto alsa = createALSABackend();
    if (alsa && alsa->isAvailable()) {
        return alsa;
    }
#endif

    return nullptr;
}

std::unique_ptr<AudioBackend> createPipeWireBackend() {
#ifdef USE_PIPEWIRE
    return std::make_unique<PipeWireBackend>();
#else
    return nullptr;
#endif
}

std::unique_ptr<AudioBackend> createALSABackend() {
#ifdef USE_ALSA
    return std::make_unique<ALSABackend>();
#else
    return nullptr;
#endif
}

} // namespace openamp
