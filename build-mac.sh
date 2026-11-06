#!/bin/bash
# Build Swell Ad-lib FX on Mac (Standalone + VST3 + AU, universal arm64/x86_64).
# Needs: Xcode command line tools (xcode-select --install) and CMake (brew install cmake).
set -e
cd "$(dirname "$0")"

cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release -DSWELL_COPY_AFTER_BUILD=ON
cmake --build build-mac --config Release --parallel

echo ""
echo "OK ! Les plugins sont copies automatiquement dans :"
echo "  AU   : ~/Library/Audio/Plug-Ins/Components/Swell Ad-lib FX.component"
echo "  VST3 : ~/Library/Audio/Plug-Ins/VST3/Swell Ad-lib FX.vst3"
echo "  App  : build-mac/SwellAdLibFX_artefacts/Release/Standalone/Swell Ad-lib FX.app"
