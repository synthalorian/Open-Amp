// Minimal test for IR Loader and Latency Monitor
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>

// Include the actual headers
#include "openamp/ir_loader.h"
#include "openamp/latency_monitor.h"

// Use openamp::AudioBuffer from plugin_interface.h
using openamp::AudioBuffer;

int main() {
    printf("=== OpenAmp Linux Validation Test ===\n\n");
    
    int failures = 0;
    
    // Test 1: LatencyMonitor basic functionality
    printf("[Test 1] LatencyMonitor...\n");
    {
        openamp::LatencyMonitor monitor;
        
        // Simulate audio callback
        monitor.markInputTime();
        std::this_thread::sleep_for(std::chrono::microseconds(500)); // 0.5ms simulated processing
        monitor.markOutputTime();
        
        float latency = monitor.getLatencyMs();
        printf("  - Current latency: %.3f ms\n", latency);
        printf("  - Average latency: %.3f ms\n", monitor.getAverageLatencyMs());
        printf("  - Sample count: %zu\n", (size_t)monitor.getSampleCount());
        
        if (latency > 0.0f && latency < 10.0f) {
            printf("  ✓ LatencyMonitor working\n");
        } else {
            printf("  ✗ LatencyMonitor failed (unexpected latency value)\n");
            failures++;
        }
        
        // Test theoretical calculation
        float theoretical = openamp::LatencyMonitor::calculateTheoreticalLatencyMs(256, 48000.0);
        printf("  - Theoretical latency (256 samples @ 48kHz): %.3f ms\n", theoretical);
        if (theoretical > 5.0f && theoretical < 6.0f) {
            printf("  ✓ Theoretical calculation correct\n");
        } else {
            printf("  ✗ Theoretical calculation failed\n");
            failures++;
        }
    }
    
    // Test 2: IRLoader initialization
    printf("\n[Test 2] IRLoader initialization...\n");
    {
        openamp::IRLoader loader;
        
        loader.prepare(48000.0, 256);
        
        if (loader.getName() == "IR Cabinet Loader") {
            printf("  ✓ IRLoader name correct\n");
        } else {
            printf("  ✗ IRLoader name incorrect: %s\n", loader.getName().c_str());
            failures++;
        }
        
        if (loader.getVersion() == "1.0.0") {
            printf("  ✓ IRLoader version correct\n");
        } else {
            printf("  ✗ IRLoader version incorrect\n");
            failures++;
        }
        
        // Check that no IR is loaded initially
        if (!loader.isIRLoaded()) {
            printf("  ✓ IRLoader starts with no IR loaded\n");
        } else {
            printf("  ✗ IRLoader should start empty\n");
            failures++;
        }
    }
    
    // Test 3: IR Loading from test file
    printf("\n[Test 3] IR Loading from test_sine_ir.wav...\n");
    {
        openamp::IRLoader loader;
        loader.prepare(48000.0, 256);
        
        std::string errorMessage;
        bool loaded = loader.loadIR("/home/synth/projects/openamp/test_ir/test_sine_ir.wav", errorMessage);
        
        if (loaded) {
            printf("  ✓ IR loaded successfully\n");
            printf("  - IR Name: %s\n", loader.getIRName().c_str());
            printf("  - IR Length: %zu samples\n", loader.getIRLength());
            printf("  - IR Sample Rate: %.0f Hz\n", loader.getIRSampleRate());
            
            // Process a buffer
            float bufferData[256] = {0};
            for (int i = 0; i < 256; i++) {
                bufferData[i] = (i < 10) ? 1.0f : 0.0f; // Simple impulse input
            }
            
            AudioBuffer buffer;
            buffer.data = bufferData;
            buffer.numChannels = 1;
            buffer.numFrames = 256;
            
            loader.process(buffer);
            
            // Check output is not zero (convolution produced output)
            float sum = 0;
            for (int i = 0; i < 256; i++) {
                sum += std::abs(bufferData[i]);
            }
            
            if (sum > 0.001f) {
                printf("  ✓ Convolution processing working (output energy: %.6f)\n", sum);
            } else {
                printf("  ✗ Convolution produced no output (sum: %.6f)\n", sum);
                failures++;
            }
            
            // Check CPU usage reporting
            float cpu = loader.getCurrentCPU();
            printf("  - CPU Usage: %.2f%%\n", cpu * 100.0f);
            
        } else {
            printf("  ✗ Failed to load IR: %s\n", errorMessage.c_str());
            failures++;
        }
    }
    
    // Test 4: Invalid file handling
    printf("\n[Test 4] IR Loading error handling...\n");
    {
        openamp::IRLoader loader;
        loader.prepare(48000.0, 256);
        
        std::string errorMessage;
        bool loaded = loader.loadIR("/nonexistent/file.wav", errorMessage);
        
        if (!loaded && !errorMessage.empty()) {
            printf("  ✓ Correctly rejected non-existent file\n");
            printf("  - Error: %s\n", errorMessage.c_str());
        } else {
            printf("  ✗ Should have failed on non-existent file\n");
            failures++;
        }
    }
    
    // Test 5: Latency with multiple samples
    printf("\n[Test 5] LatencyMonitor statistics...\n");
    {
        openamp::LatencyMonitor monitor;
        monitor.reset();
        
        // Generate 10 samples
        for (int i = 0; i < 10; i++) {
            monitor.markInputTime();
            std::this_thread::sleep_for(std::chrono::microseconds(100 + i * 50)); // Varying latency
            monitor.markOutputTime();
        }
        
        float avgLatency = monitor.getAverageLatencyMs();
        float lastLatency = monitor.getLatencyMs();
        uint64_t count = monitor.getSampleCount();
        
        printf("  - Samples collected: %zu\n", (size_t)count);
        printf("  - Average latency: %.3f ms\n", avgLatency);
        printf("  - Last latency: %.3f ms\n", lastLatency);
        
        if (count == 10 && avgLatency > 0.0f) {
            printf("  ✓ Statistics collection working\n");
        } else {
            printf("  ✗ Statistics collection failed\n");
            failures++;
        }
    }
    
    printf("\n=== Test Summary ===\n");
    if (failures == 0) {
        printf("✓ All tests passed!\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", failures);
        return 1;
    }
}
