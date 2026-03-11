#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "plugins/delay/delay.h"
#include <vector>
#include <cmath>

using Catch::Approx;
using namespace openamp;

TEST_CASE("Delay - Construction", "[delay]") {
    Delay delay;

    SECTION("Has correct name") {
        REQUIRE(delay.getName() == "Delay");
    }

    SECTION("Has version") {
        REQUIRE(delay.getVersion() == "1.0.0");
    }
}

TEST_CASE("Delay - Prepare and process", "[delay]") {
    Delay delay;
    delay.prepare(48000.0, 256);
    delay.setTimeMs(100.0f);
    delay.setMix(0.5f);
    delay.setFeedback(0.3f);

    SECTION("Process silence") {
        std::vector<float> buffer(256, 0.0f);
        AudioBuffer buf{buffer.data(), 1, 256, 48000};

        delay.process(buf);

        float maxVal = 0.0f;
        for (float s : buffer) {
            maxVal = std::max(maxVal, std::abs(s));
        }
        REQUIRE(maxVal < 0.001f);
    }

    SECTION("Process signal") {
        std::vector<float> buffer(256, 0.0f);
        buffer[0] = 1.0f; // Impulse

        AudioBuffer buf{buffer.data(), 1, 256, 48000};
        delay.process(buf);

        // First sample should have dry signal
        REQUIRE(std::abs(buffer[0]) > 0.0f);
    }
}

TEST_CASE("Delay - Time control", "[delay]") {
    Delay delay;
    delay.prepare(48000.0, 256);

    SECTION("Short delay") {
        REQUIRE_NOTHROW(delay.setTimeMs(10.0f));
    }

    SECTION("Medium delay") {
        REQUIRE_NOTHROW(delay.setTimeMs(350.0f));
    }

    SECTION("Long delay") {
        REQUIRE_NOTHROW(delay.setTimeMs(1000.0f));
    }
}

TEST_CASE("Delay - Feedback control", "[delay]") {
    Delay delay;
    delay.prepare(48000.0, 256);

    REQUIRE_NOTHROW(delay.setFeedback(0.0f));
    REQUIRE_NOTHROW(delay.setFeedback(0.5f));
    REQUIRE_NOTHROW(delay.setFeedback(0.9f));
}

TEST_CASE("Delay - Mix control", "[delay]") {
    Delay delay;
    delay.prepare(48000.0, 256);

    REQUIRE_NOTHROW(delay.setMix(0.0f));
    REQUIRE_NOTHROW(delay.setMix(0.5f));
    REQUIRE_NOTHROW(delay.setMix(1.0f));
}

TEST_CASE("Delay - Impulse response", "[delay]") {
    Delay delay;
    delay.prepare(48000.0, 256);
    delay.setTimeMs(50.0f); // 50ms = 2400 samples at 48kHz, but we'll use shorter
    delay.setFeedback(0.0f);
    delay.setMix(1.0f); // 100% wet to hear the delay

    // Actually, let's use a delay that fits in our buffer
    delay.setTimeMs(5.0f); // 5ms ≈ 240 samples

    // Create impulse
    size_t delaySamples = static_cast<size_t>(5.0f * 48000.0f / 1000.0f);
    std::vector<float> buffer(1000, 0.0f);
    buffer[0] = 1.0f; // Impulse at start

    AudioBuffer buf{buffer.data(), 1, static_cast<uint32_t>(buffer.size()), 48000};
    delay.process(buf);

    // With 100% wet mix, we should hear the delayed impulse
    // (though exact behavior depends on implementation)
    bool hasSignal = false;
    for (float s : buffer) {
        if (std::abs(s) > 0.01f) {
            hasSignal = true;
            break;
        }
    }
    REQUIRE(hasSignal);
}

TEST_CASE("Delay - Reset", "[delay]") {
    Delay delay;
    delay.prepare(48000.0, 256);

    std::vector<float> buffer(256, 0.5f);
    AudioBuffer buf{buffer.data(), 1, 256, 48000};
    delay.process(buf);

    REQUIRE_NOTHROW(delay.reset());
    REQUIRE_NOTHROW(delay.process(buf));
}
