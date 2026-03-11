#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "plugins/reverb/reverb.h"
#include <vector>
#include <cmath>

using Catch::Approx;
using namespace openamp;

TEST_CASE("Reverb - Construction", "[reverb]") {
    Reverb reverb;

    SECTION("Has correct name") {
        REQUIRE(reverb.getName() == "Reverb");
    }

    SECTION("Has version") {
        REQUIRE(reverb.getVersion() == "1.0.0");
    }
}

TEST_CASE("Reverb - Prepare and process", "[reverb]") {
    Reverb reverb;
    reverb.prepare(48000.0, 256);

    SECTION("Process silence") {
        std::vector<float> buffer(256, 0.0f);
        AudioBuffer buf{buffer.data(), 1, 256, 48000};

        reverb.process(buf);

        float maxVal = 0.0f;
        for (float s : buffer) {
            maxVal = std::max(maxVal, std::abs(s));
        }
        REQUIRE(maxVal < 0.001f);
    }

    SECTION("Process signal") {
        std::vector<float> buffer(256);
        for (size_t i = 0; i < buffer.size(); ++i) {
            buffer[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * i / 48000.0);
        }

        AudioBuffer buf{buffer.data(), 1, 256, 48000};
        reverb.process(buf);

        // Reverb should add tails/smear the signal
        bool hasSignal = false;
        for (float s : buffer) {
            if (std::abs(s) > 0.01f) {
                hasSignal = true;
                break;
            }
        }
        REQUIRE(hasSignal);
    }
}

TEST_CASE("Reverb - Room size control", "[reverb]") {
    Reverb reverb;
    reverb.prepare(48000.0, 256);

    REQUIRE_NOTHROW(reverb.setRoomSize(0.0f));
    REQUIRE_NOTHROW(reverb.setRoomSize(0.5f));
    REQUIRE_NOTHROW(reverb.setRoomSize(1.0f));
}

TEST_CASE("Reverb - Damping control", "[reverb]") {
    Reverb reverb;
    reverb.prepare(48000.0, 256);

    REQUIRE_NOTHROW(reverb.setDamping(0.0f));
    REQUIRE_NOTHROW(reverb.setDamping(0.5f));
    REQUIRE_NOTHROW(reverb.setDamping(1.0f));
}

TEST_CASE("Reverb - Mix control", "[reverb]") {
    Reverb reverb;
    reverb.prepare(48000.0, 256);

    REQUIRE_NOTHROW(reverb.setMix(0.0f));
    REQUIRE_NOTHROW(reverb.setMix(0.5f));
    REQUIRE_NOTHROW(reverb.setMix(1.0f));
}

TEST_CASE("Reverb - Dry signal preserved at low mix", "[reverb]") {
    Reverb reverb;
    reverb.prepare(48000.0, 256);
    reverb.setMix(0.0f); // 100% dry

    std::vector<float> buffer(256);
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = 0.5f;
    }
    std::vector<float> original = buffer;

    AudioBuffer buf{buffer.data(), 1, 256, 48000};
    reverb.process(buf);

    // At 0% mix, dry signal should be preserved
    for (size_t i = 0; i < buffer.size(); ++i) {
        REQUIRE(buffer[i] == Approx(original[i]).margin(0.01f));
    }
}

TEST_CASE("Reverb - Reset", "[reverb]") {
    Reverb reverb;
    reverb.prepare(48000.0, 256);

    std::vector<float> buffer(256, 0.5f);
    AudioBuffer buf{buffer.data(), 1, 256, 48000};
    reverb.process(buf);

    REQUIRE_NOTHROW(reverb.reset());
    REQUIRE_NOTHROW(reverb.process(buf));
}
