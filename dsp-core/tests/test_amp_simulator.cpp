#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "openamp/amp_simulator.h"
#include <vector>
#include <cmath>

using Catch::Approx;
using namespace openamp;

TEST_CASE("AmpSimulator - Construction", "[amp_simulator]") {
    AmpSimulator amp;

    SECTION("Has correct name") {
        REQUIRE(amp.getName() == "Amp Simulator");
    }

    SECTION("Has version") {
        REQUIRE(amp.getVersion() == "1.0.0");
    }
}

TEST_CASE("AmpSimulator - Prepare and process", "[amp_simulator]") {
    AmpSimulator amp;
    amp.prepare(48000.0, 256);

    SECTION("Process silence") {
        std::vector<float> buffer(256, 0.0f);
        AudioBuffer buf{buffer.data(), 1, 256, 48000};

        amp.process(buf);

        float maxVal = 0.0f;
        for (float s : buffer) {
            maxVal = std::max(maxVal, std::abs(s));
        }
        REQUIRE(maxVal < 0.001f);
    }

    SECTION("Process signal") {
        std::vector<float> buffer(256);
        for (size_t i = 0; i < buffer.size(); ++i) {
            buffer[i] = 0.1f * std::sin(2.0 * M_PI * 440.0 * i / 48000.0);
        }
        std::vector<float> original = buffer;

        AudioBuffer buf{buffer.data(), 1, 256, 48000};
        amp.process(buf);

        // Output should be different (amplified/processed)
        bool changed = false;
        for (size_t i = 0; i < buffer.size(); ++i) {
            if (std::abs(buffer[i] - original[i]) > 0.001f) {
                changed = true;
                break;
            }
        }
        REQUIRE(changed);
    }
}

TEST_CASE("AmpSimulator - Gain control", "[amp_simulator]") {
    AmpSimulator amp;
    amp.prepare(48000.0, 256);

    SECTION("Positive gain amplifies") {
        amp.setGain(12.0f); // +12dB ≈ 4x

        std::vector<float> buffer(256, 0.1f);
        AudioBuffer buf{buffer.data(), 1, 256, 48000};
        amp.process(buf);

        float avgOut = 0.0f;
        for (float s : buffer) avgOut += std::abs(s);
        avgOut /= buffer.size();

        REQUIRE(avgOut > 0.1f); // Should be amplified
    }

    SECTION("Negative gain attenuates") {
        amp.setGain(-12.0f); // -12dB ≈ 0.25x

        std::vector<float> buffer(256, 0.5f);
        AudioBuffer buf{buffer.data(), 1, 256, 48000};
        amp.process(buf);

        float maxOut = 0.0f;
        for (float s : buffer) maxOut = std::max(maxOut, std::abs(s));
        REQUIRE(maxOut < 0.5f); // Should be attenuated
    }
}

TEST_CASE("AmpSimulator - Drive control", "[amp_simulator]") {
    AmpSimulator amp;
    amp.prepare(48000.0, 256);

    SECTION("Low drive") {
        amp.setDrive(0.2f);
        std::vector<float> buffer(256, 0.5f);
        AudioBuffer buf{buffer.data(), 1, 256, 48000};

        REQUIRE_NOTHROW(amp.process(buf));
    }

    SECTION("High drive") {
        amp.setDrive(0.9f);
        std::vector<float> buffer(256, 0.5f);
        AudioBuffer buf{buffer.data(), 1, 256, 48000};

        REQUIRE_NOTHROW(amp.process(buf));
    }
}

TEST_CASE("AmpSimulator - EQ controls", "[amp_simulator]") {
    AmpSimulator amp;
    amp.prepare(48000.0, 256);

    SECTION("Bass control") {
        REQUIRE_NOTHROW(amp.setBass(6.0f));
        REQUIRE_NOTHROW(amp.setBass(-6.0f));
    }

    SECTION("Mid control") {
        REQUIRE_NOTHROW(amp.setMid(6.0f));
        REQUIRE_NOTHROW(amp.setMid(-6.0f));
    }

    SECTION("Treble control") {
        REQUIRE_NOTHROW(amp.setTreble(6.0f));
        REQUIRE_NOTHROW(amp.setTreble(-6.0f));
    }

    SECTION("Presence control") {
        REQUIRE_NOTHROW(amp.setPresence(6.0f));
        REQUIRE_NOTHROW(amp.setPresence(-6.0f));
    }
}

TEST_CASE("AmpSimulator - Master control", "[amp_simulator]") {
    AmpSimulator amp;
    amp.prepare(48000.0, 256);

    SECTION("Master affects output level") {
        amp.setMaster(0.0f); // 0dB

        std::vector<float> buffer1(256, 0.3f);
        AudioBuffer buf1{buffer1.data(), 1, 256, 48000};
        amp.process(buf1);
        float level1 = 0.0f;
        for (float s : buffer1) level1 += std::abs(s);

        amp.reset();
        amp.setMaster(-20.0f); // -20dB

        std::vector<float> buffer2(256, 0.3f);
        AudioBuffer buf2{buffer2.data(), 1, 256, 48000};
        amp.process(buf2);
        float level2 = 0.0f;
        for (float s : buffer2) level2 += std::abs(s);

        REQUIRE(level2 < level1); // Lower master = lower output
    }
}

TEST_CASE("AmpSimulator - Reset", "[amp_simulator]") {
    AmpSimulator amp;
    amp.prepare(48000.0, 256);

    // Process some audio
    std::vector<float> buffer(256, 0.5f);
    AudioBuffer buf{buffer.data(), 1, 256, 48000};
    amp.process(buf);

    // Reset should not crash
    REQUIRE_NOTHROW(amp.reset());

    // Can process again after reset
    REQUIRE_NOTHROW(amp.process(buf));
}
