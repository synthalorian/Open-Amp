#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "openamp/preset_store.h"
#include <fstream>
#include <cstdio>

using Catch::Approx;
using namespace openamp;

TEST_CASE("PresetStore - Save and load preset", "[preset_store]") {
    const char* testFile = "/tmp/test_preset.preset";

    SECTION("Save preset") {
        Preset preset;
        preset.name = "Test Preset";
        preset.inputGainDb = -6.0f;
        preset.ampEnabled = true;
        preset.ampGainDb = 3.5f;
        preset.ampDrive = 0.75f;
        preset.delayEnabled = true;
        preset.delayTimeMs = 400.0f;
        preset.reverbEnabled = true;
        preset.reverbRoom = 0.6f;

        std::string error;
        bool saved = PresetStore::savePreset(preset, testFile, error);

        REQUIRE(saved);
        REQUIRE(error.empty());

        // Verify file exists
        std::ifstream file(testFile);
        REQUIRE(file.good());
    }

    SECTION("Load preset") {
        // First save a preset
        Preset original;
        original.name = "Load Test";
        original.ampGainDb = 8.0f;
        original.ampDrive = 0.65f;
        original.delayTimeMs = 300.0f;

        std::string error;
        PresetStore::savePreset(original, testFile, error);

        // Now load it
        Preset loaded;
        bool loadedOk = PresetStore::loadPreset(testFile, loaded, error);

        REQUIRE(loadedOk);
        REQUIRE(loaded.name == "Load Test");
        REQUIRE(loaded.ampGainDb == Approx(8.0f));
        REQUIRE(loaded.ampDrive == Approx(0.65f));
        REQUIRE(loaded.delayTimeMs == Approx(300.0f));
    }

    SECTION("Round trip preserves data") {
        Preset original;
        original.name = "Round Trip Test";
        original.inputGainDb = -3.0f;
        original.outputGainDb = 2.0f;
        original.ampEnabled = true;
        original.effectsEnabled = true;
        original.delayEnabled = false;
        original.reverbEnabled = true;
        original.distortionEnabled = true;
        original.delayFirst = false;
        original.delayTimeMs = 500.0f;
        original.delayFeedback = 0.45f;
        original.delayMix = 0.3f;
        original.reverbRoom = 0.7f;
        original.reverbDamp = 0.4f;
        original.reverbMix = 0.25f;
        original.distortionDrive = 0.8f;
        original.distortionTone = 0.6f;
        original.distortionLevel = 0.75f;
        original.distortionType = 2;
        original.ampGainDb = 5.0f;
        original.ampDrive = 0.55f;
        original.ampBassDb = 2.0f;
        original.ampMidDb = -1.0f;
        original.ampTrebleDb = 3.0f;
        original.ampPresenceDb = 1.0f;
        original.ampMasterDb = 0.0f;
        original.cabIrPath = "/path/to/ir.wav";

        std::string error;
        REQUIRE(PresetStore::savePreset(original, testFile, error));

        Preset loaded;
        REQUIRE(PresetStore::loadPreset(testFile, loaded, error));

        REQUIRE(loaded.name == original.name);
        REQUIRE(loaded.inputGainDb == Approx(original.inputGainDb));
        REQUIRE(loaded.outputGainDb == Approx(original.outputGainDb));
        REQUIRE(loaded.ampEnabled == original.ampEnabled);
        REQUIRE(loaded.effectsEnabled == original.effectsEnabled);
        REQUIRE(loaded.delayEnabled == original.delayEnabled);
        REQUIRE(loaded.reverbEnabled == original.reverbEnabled);
        REQUIRE(loaded.distortionEnabled == original.distortionEnabled);
        REQUIRE(loaded.delayFirst == original.delayFirst);
        REQUIRE(loaded.delayTimeMs == Approx(original.delayTimeMs));
        REQUIRE(loaded.delayFeedback == Approx(original.delayFeedback));
        REQUIRE(loaded.delayMix == Approx(original.delayMix));
        REQUIRE(loaded.reverbRoom == Approx(original.reverbRoom));
        REQUIRE(loaded.reverbDamp == Approx(original.reverbDamp));
        REQUIRE(loaded.reverbMix == Approx(original.reverbMix));
        REQUIRE(loaded.distortionDrive == Approx(original.distortionDrive));
        REQUIRE(loaded.distortionTone == Approx(original.distortionTone));
        REQUIRE(loaded.distortionLevel == Approx(original.distortionLevel));
        REQUIRE(loaded.distortionType == original.distortionType);
        REQUIRE(loaded.ampGainDb == Approx(original.ampGainDb));
        REQUIRE(loaded.ampDrive == Approx(original.ampDrive));
        REQUIRE(loaded.ampBassDb == Approx(original.ampBassDb));
        REQUIRE(loaded.ampMidDb == Approx(original.ampMidDb));
        REQUIRE(loaded.ampTrebleDb == Approx(original.ampTrebleDb));
        REQUIRE(loaded.ampPresenceDb == Approx(original.ampPresenceDb));
        REQUIRE(loaded.ampMasterDb == Approx(original.ampMasterDb));
        REQUIRE(loaded.cabIrPath == original.cabIrPath);
    }

    // Cleanup
    std::remove(testFile);
}

TEST_CASE("PresetStore - Error handling", "[preset_store]") {
    SECTION("Load non-existent file") {
        Preset preset;
        std::string error;
        bool loaded = PresetStore::loadPreset("/nonexistent/path.preset", preset, error);

        REQUIRE_FALSE(loaded);
        REQUIRE_FALSE(error.empty());
    }

    SECTION("Save to invalid path") {
        Preset preset;
        preset.name = "Test";
        std::string error;
        bool saved = PresetStore::savePreset(preset, "/nonexistent/dir/preset.preset", error);

        REQUIRE_FALSE(saved);
        REQUIRE_FALSE(error.empty());
    }
}

TEST_CASE("Preset - Default values", "[preset_store]") {
    Preset preset;

    REQUIRE(preset.name.empty());
    REQUIRE(preset.inputGainDb == Approx(0.0f));
    REQUIRE(preset.ampEnabled == true);
    REQUIRE(preset.delayEnabled == true);
    REQUIRE(preset.reverbEnabled == true);
    REQUIRE(preset.distortionEnabled == false);
}
