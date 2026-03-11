#!/bin/bash

# Build script for Guitar Amp Linux

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "🎸 Building Guitar Amp for Linux..."

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure
echo "📦 Configuring..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_PIPEWIRE=ON \
    -DUSE_ALSA=ON \
    -DBUILD_TESTS=OFF

# Build
echo "🔨 Compiling..."
make -j$(nproc)

echo "✅ Build complete!"
echo ""
echo "Run with: ${BUILD_DIR}/guitar-amp-linux"
