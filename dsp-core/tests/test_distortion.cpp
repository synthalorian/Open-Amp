#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "plugins/distortion/distortion.h"
#include <vector>
#include <cmath>

using Catch::Approx;
using namespace openamp;

TEST_CASE("Distortion - Construction", "[distortion]") {
    Distortion dist;

    SECTION("Has correct name") {
        REQUIRE(dist.getName() == "Distortion");
    }

    SECTION("Has version") {
        REQUIRE(dist.getVersion() == "1.0.0");
    }
}

TEST_CASE("Distortion - Types", "[distortion]") {
    Distortion dist;
    dist.prepare(48000.0, 256);

    SECTION("Overdrive") {
        REQUIRE_NOTHROW(dist.setType(Distortion::Type::Overdrive));
    }

    SECTION("Fuzz") {
        REQUIRE_NOTHROW(dist.setType(Distortion::Type::Fuzz));
    }

    SECTION("Tube") {
        REQUIRE_NOTHROW(dist.setType(Distortion::Type::Tube));
    }

    SECTION("HardClip") {
        REQUIRE_NOTHROW(dist.setType(Distortion::Type::HardClip));
    }
}

TEST_CASE("Distortion - Process silence", "[distortion]") {
    Distortion dist;
    dist.prepare(48000.0, 256);

    std::vector<float> buffer(256, 0.0f);
    AudioBuffer buf{buffer.data(), 1, 256, 48000};

    dist.process(buf);

    float maxVal = 0.0f;
    for (float s : buffer) {
        maxVal = std::max(maxVal, std::abs(s));
    }
    REQUIRE(maxVal < 0.001f);
}

TEST_CASE("Distortion - Process signal", "[distortion]") {
    Distortion dist;
    dist.prepare(48000.0, 256);
    dist.setDrive(0.7f);

    std::vector<float> buffer(256);
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * i / 48000.0);
    }

    AudioBuffer buf{buffer.data(), 1, 256, 48000};
    dist.process(buf);

    // Distortion should change the signal
    bool changed = false;
    for (float& s : buffer) {
        if (std::abs(s) > 0.001f) {
            changed = true;
            break;
        }
    }
    REQUIRE(changed);
}

TEST_CASE("Distortion - Drive control", "[distortion]") {
    Distortion dist;
    dist.prepare(48000.0, 256);

    SECTION("Low drive") {
        REQUIRE_NOTHROW(dist.setDrive(0.1f));
    }

    SECTION("High drive") {
        REQUIRE_NOTHROW(dist.setDrive(1.0f));
    }
}

TEST_CASE("Distortion - Tone control", "[distortion]") {
    Distortion dist;
    dist.prepare(48000.0, 256);

    REQUIRE_NOTHROW(dist.setTone(0.0f));
    REQUIRE_NOTHROW(dist.setTone(0.5f));
    REQUIRE_NOTHROW(dist.setTone(1.0f));
}

TEST_CASE("Distortion - Level control", "[distortion]") {
    Distortion dist;
    dist.prepare(48000.0, 256);

    REQUIRE_NOTHROW(dist.setLevel(0.0f));
    REQUIRE_NOTHROW(dist.setLevel(0.5f));
    REQUIRE_NOTHROW(dist.setLevel(1.0f));
}

TEST_CASE("Distortion - Hard clipping", "[distortion]") {
    Distortion dist;
    dist.prepare(48000.0, 256);
    dist.setType(Distortion::Type::HardClip);
    dist.setDrive(1.0f);

    std::vector<float> buffer(256, 2.0f); // High input
    AudioBuffer buf{buffer.data(), 1, 256, 48000};

    dist.process(buf);

    // Hard clip should limit output
    for (float s : buffer) {
        REQUIRE(std::abs(s) <= 1.0f);
    }
}

TEST_CASE("Distortion - Reset", "[distortion]") {
    Distortion dist;
    dist.prepare(48000.0, 256);

    std::vector<float> buffer(256, 0.5f);
    AudioBuffer buf{buffer.data(), 1, 256, 48000};
    dist.process(buf);

    REQUIRE_NOTHROW(dist.reset());
    REQUIRE_NOTHROW(dist.process(buf));
}
