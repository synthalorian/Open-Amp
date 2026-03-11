#pragma once

#include <cstdint>
#include <chrono>

namespace openamp {

/**
 * Latency Monitor - Tracks round-trip audio latency
 */
class LatencyMonitor {
public:
    LatencyMonitor() = default;

    // Call when audio input is received
    void markInputTime() {
        inputTime_ = std::chrono::high_resolution_clock::now();
        inputMarked_ = true;
    }

    // Call when corresponding output is sent
    void markOutputTime() {
        if (inputMarked_) {
            auto outputTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                outputTime - inputTime_);
            
            lastLatencyUs_ = duration.count();
            totalLatencyUs_ += lastLatencyUs_;
            sampleCount_++;
            
            inputMarked_ = false;
        }
    }

    // Get current latency in milliseconds
    float getLatencyMs() const {
        return lastLatencyUs_ / 1000.0f;
    }

    // Get average latency in milliseconds
    float getAverageLatencyMs() const {
        if (sampleCount_ == 0) return 0.0f;
        return (totalLatencyUs_ / sampleCount_) / 1000.0f;
    }

    // Get theoretical latency based on buffer size and sample rate
    static float calculateTheoreticalLatencyMs(uint32_t bufferSize, double sampleRate) {
        return (bufferSize / sampleRate) * 1000.0f;
    }

    // Reset statistics
    void reset() {
        lastLatencyUs_ = 0;
        totalLatencyUs_ = 0;
        sampleCount_ = 0;
        inputMarked_ = false;
    }

    // Get statistics
    uint64_t getSampleCount() const { return sampleCount_; }
    uint64_t getLastLatencyUs() const { return lastLatencyUs_; }

private:
    std::chrono::high_resolution_clock::time_point inputTime_;
    bool inputMarked_ = false;
    uint64_t lastLatencyUs_ = 0;
    uint64_t totalLatencyUs_ = 0;
    uint64_t sampleCount_ = 0;
};

} // namespace openamp
