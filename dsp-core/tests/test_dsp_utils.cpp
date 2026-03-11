#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "openamp/dsp_utils.h"
#include <cmath>

using Catch::Approx;
using namespace openamp;

TEST_CASE("OnePoleFilter - Initial state", "[dsp_utils]") {
    OnePoleFilter filter;

    SECTION("Default output is zero") {
        REQUIRE(filter.process(0.0f) == Approx(0.0f));
    }

    SECTION("Can process signal") {
        float output = filter.process(1.0f);
        // Lowpass should smooth the input
        REQUIRE(output >= 0.0f);
        REQUIRE(output <= 1.0f);
    }
}

TEST_CASE("OnePoleFilter - Set cutoff", "[dsp_utils]") {
    OnePoleFilter filter;
    filter.setCutoff(1000.0f, 48000.0f);

    // Process some samples
    float sum = 0.0f;
    for (int i = 0; i < 100; ++i) {
        sum += filter.process(1.0f);
    }

    // After 100 samples, output should have converged toward input
    REQUIRE(sum > 10.0f);
}

TEST_CASE("OnePoleFilter - Reset", "[dsp_utils]") {
    OnePoleFilter filter;

    // Process some signal
    for (int i = 0; i < 50; ++i) {
        filter.process(1.0f);
    }

    // Reset
    filter.reset();

    // Should be back to initial state
    REQUIRE(filter.process(0.0f) == Approx(0.0f));
}

TEST_CASE("dbToLinear and linearToDb", "[dsp_utils]") {
    SECTION("0 dB = 1.0 linear") {
        REQUIRE(dbToLinear(0.0f) == Approx(1.0f));
    }

    SECTION("6 dB ≈ 2.0 linear") {
        REQUIRE(dbToLinear(6.0f) == Approx(2.0f).margin(0.1));
    }

    SECTION("-6 dB ≈ 0.5 linear") {
        REQUIRE(dbToLinear(-6.0f) == Approx(0.5f).margin(0.05));
    }

    SECTION("Round trip") {
        float originalDb = 12.0f;
        float linear = dbToLinear(originalDb);
        float backToDb = linearToDb(linear);
        REQUIRE(backToDb == Approx(originalDb).margin(0.01));
    }
}

TEST_CASE("clamp function", "[dsp_utils]") {
    REQUIRE(clamp(0.5f, 0.0f, 1.0f) == Approx(0.5f));
    REQUIRE(clamp(-1.0f, 0.0f, 1.0f) == Approx(0.0f));
    REQUIRE(clamp(2.0f, 0.0f, 1.0f) == Approx(1.0f));
    REQUIRE(clamp(50.0f, 0.0f, 100.0f) == Approx(50.0f));
}

TEST_CASE("lerp function", "[dsp_utils]") {
    REQUIRE(lerp(0.0f, 10.0f, 0.0f) == Approx(0.0f));
    REQUIRE(lerp(0.0f, 10.0f, 1.0f) == Approx(10.0f));
    REQUIRE(lerp(0.0f, 10.0f, 0.5f) == Approx(5.0f));
    REQUIRE(lerp(-10.0f, 10.0f, 0.5f) == Approx(0.0f));
}
