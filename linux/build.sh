#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="build"
echo "🎸 Building Open Amp for Linux..."
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_PIPEWIRE=ON \
    -DUSE_ALSA=ON \
    -DBUILD_TESTS=OFF
make -j"$(nproc)"
echo "✅ Build complete!"
echo "Run: ${SCRIPT_DIR}/${BUILD_DIR}/openamp"
