#include "openamp/ir_loader.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace openamp {

IRLoader::IRLoader() = default;
IRLoader::~IRLoader() = default;

void IRLoader::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlockSize_ = maxBlockSize;
    
    // Allocate history buffer for convolution
    if (!ir_.empty()) {
        historyBuffer_.resize(ir_.size() + maxBlockSize, 0.0f);
    }
    
    // Setup filters
    highCut_.setCutoff(highCutHz_, static_cast<float>(sampleRate));
    lowCut_.setCutoff(lowCutHz_, static_cast<float>(sampleRate));
    
    prepared_ = true;
}

void IRLoader::process(AudioBuffer& buffer) {
    if (!enabled_ || ir_.empty() || !prepared_) {
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
        float* channelData = buffer.data + ch * buffer.numFrames;
        
        // Process each sample
        for (uint32_t i = 0; i < buffer.numFrames; ++i) {
            // Add input to history buffer with input gain
            float inputSample = channelData[i] * inputGain_;
            historyBuffer_[historyIndex_] = inputSample;
            
            // Convolve with IR (simple direct convolution)
            float output = 0.0f;
            for (size_t j = 0; j < ir_.size(); ++j) {
                size_t historyIdx = (historyIndex_ - j + historyBuffer_.size()) % historyBuffer_.size();
                output += historyBuffer_[historyIdx] * ir_[j];
            }
            
            // Apply filters
            output = highCut_.process(output);
            output = lowCut_.process(output);
            output *= outputGain_;
            
            // Mix with dry signal
            channelData[i] = channelData[i] * (1.0f - mix_) + output * mix_;
            
            // Advance history index
            historyIndex_ = (historyIndex_ + 1) % historyBuffer_.size();
        }
    }
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    processTimeUs_ += std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    processCount_++;
    
    if (processCount_ >= 100) {
        float framesProcessed = processCount_ * buffer.numFrames;
        float secondsProcessed = framesProcessed / sampleRate_;
        cpuUsage_ = (processTimeUs_ / 1000000.0f) / secondsProcessed;
        processTimeUs_ = 0;
        processCount_ = 0;
    }
}

void IRLoader::reset() {
    std::fill(historyBuffer_.begin(), historyBuffer_.end(), 0.0f);
    historyIndex_ = 0;
    highCut_.reset();
    lowCut_.reset();
}

bool IRLoader::loadIR(const std::string& path, std::string& errorMessage) {
    std::vector<float> samples;
    WAVHeader header;
    
    if (!parseWAV(path, samples, header, errorMessage)) {
        return false;
    }

    // Convert to mono if stereo
    std::vector<float> monoSamples;
    if (header.numChannels == 2) {
        monoSamples.reserve(samples.size() / 2);
        for (size_t i = 0; i + 1 < samples.size(); i += 2) {
            monoSamples.push_back((samples[i] + samples[i + 1]) * 0.5f);
        }
        samples = std::move(monoSamples);
    }

    // Resample if needed
    if (header.sampleRate != static_cast<uint32_t>(sampleRate_)) {
        std::vector<float> resampled;
        resampleIR(samples, header.sampleRate, resampled, sampleRate_);
        samples = std::move(resampled);
    }

    // Preprocess
    trimIR(samples);
    normalizeIR(samples);

    // Store
    ir_ = std::move(samples);
    irLength_ = ir_.size();
    irName_ = path.substr(path.find_last_of("/\\") + 1);
    irSampleRate_ = sampleRate_;

    // Reallocate history buffer
    if (prepared_) {
        historyBuffer_.resize(ir_.size() + maxBlockSize_, 0.0f);
        historyIndex_ = 0;
    }

    return true;
}

bool IRLoader::parseWAV(const std::string& filePath, std::vector<float>& samples,
                        WAVHeader& header, std::string& errorMessage) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        errorMessage = "Cannot open file: " + filePath;
        return false;
    }

    // Read RIFF header
    char riff[4];
    file.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) {
        errorMessage = "Not a valid WAV file (no RIFF header)";
        return false;
    }

    uint32_t fileSize;
    file.read(reinterpret_cast<char*>(&fileSize), 4);

    char wave[4];
    file.read(wave, 4);
    if (std::strncmp(wave, "WAVE", 4) != 0) {
        errorMessage = "Not a valid WAV file (no WAVE format)";
        return false;
    }

    // Find fmt and data chunks
    bool foundFmt = false;
    bool foundData = false;
    
    while (file && (!foundFmt || !foundData)) {
        char chunkId[4];
        uint32_t chunkSize;
        
        file.read(chunkId, 4);
        if (!file) break;
        
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (!file) break;

        if (std::strncmp(chunkId, "fmt ", 4) == 0) {
            uint16_t audioFormat;
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            
            if (audioFormat != 1) {
                errorMessage = "Only PCM format supported (not compressed)";
                return false;
            }

            file.read(reinterpret_cast<char*>(&header.numChannels), 2);
            file.read(reinterpret_cast<char*>(&header.sampleRate), 4);
            
            uint32_t byteRate;
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            
            uint16_t blockAlign;
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            
            file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2);
            
            // Skip extra format bytes
            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
            
            foundFmt = true;
        } else if (std::strncmp(chunkId, "data", 4) == 0) {
            header.dataSize = chunkSize;
            
            size_t numSamples = header.dataSize / (header.bitsPerSample / 8);
            samples.resize(numSamples);
            
            if (header.bitsPerSample == 16) {
                std::vector<int16_t> rawSamples(numSamples);
                file.read(reinterpret_cast<char*>(rawSamples.data()), header.dataSize);
                for (size_t i = 0; i < numSamples; ++i) {
                    samples[i] = rawSamples[i] / 32768.0f;
                }
            } else if (header.bitsPerSample == 24) {
                for (size_t i = 0; i < numSamples; ++i) {
                    uint8_t bytes[3];
                    file.read(reinterpret_cast<char*>(bytes), 3);
                    int32_t sample = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16);
                    if (sample & 0x800000) sample |= 0xFF000000; // Sign extend
                    samples[i] = sample / 8388608.0f;
                }
            } else if (header.bitsPerSample == 32) {
                std::vector<int32_t> rawSamples(numSamples);
                file.read(reinterpret_cast<char*>(rawSamples.data()), header.dataSize);
                for (size_t i = 0; i < numSamples; ++i) {
                    samples[i] = rawSamples[i] / 2147483648.0f;
                }
            } else {
                errorMessage = "Unsupported bit depth: " + std::to_string(header.bitsPerSample);
                return false;
            }
            
            foundData = true;
        } else {
            // Skip unknown chunk
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!foundFmt || !foundData) {
        errorMessage = "Invalid WAV file structure";
        return false;
    }

    return true;
}

void IRLoader::resampleIR(const std::vector<float>& input, double inputRate,
                          std::vector<float>& output, double outputRate) {
    if (inputRate == outputRate) {
        output = input;
        return;
    }

    double ratio = inputRate / outputRate;
    size_t outputSize = static_cast<size_t>(input.size() / ratio);
    output.resize(outputSize);

    // Linear interpolation
    for (size_t i = 0; i < outputSize; ++i) {
        double srcIdx = i * ratio;
        size_t idx0 = static_cast<size_t>(srcIdx);
        size_t idx1 = std::min(idx0 + 1, input.size() - 1);
        double frac = srcIdx - idx0;
        
        output[i] = input[idx0] * static_cast<float>(1.0 - frac) + input[idx1] * static_cast<float>(frac);
    }
}

void IRLoader::trimIR(std::vector<float>& ir) {
    // Remove leading/trailing silence below -60dB
    const float threshold = 0.001f; // -60dB
    
    size_t start = 0;
    while (start < ir.size() && std::abs(ir[start]) < threshold) {
        start++;
    }
    
    size_t end = ir.size();
    while (end > start && std::abs(ir[end - 1]) < threshold) {
        end--;
    }
    
    if (start > 0 || end < ir.size()) {
        ir = std::vector<float>(ir.begin() + start, ir.begin() + end);
    }
}

void IRLoader::normalizeIR(std::vector<float>& ir) {
    // Find peak
    float peak = 0.0f;
    for (const auto& sample : ir) {
        float absVal = std::abs(sample);
        if (absVal > peak) peak = absVal;
    }
    
    // Normalize to -0.1dB (0.989)
    if (peak > 0.0f) {
        float scale = 0.989f / peak;
        for (auto& sample : ir) {
            sample *= scale;
        }
    }
}

void IRLoader::setMix(float mix) {
    mix_ = std::clamp(mix, 0.0f, 1.0f);
}

void IRLoader::setInputGain(float gainDb) {
    inputGain_ = std::pow(10.0f, gainDb / 20.0f);
}

void IRLoader::setOutputGain(float gainDb) {
    outputGain_ = std::pow(10.0f, gainDb / 20.0f);
}

void IRLoader::setEnabled(bool enabled) {
    enabled_ = enabled;
}

void IRLoader::setHighCut(float hz) {
    highCutHz_ = std::max(1000.0f, std::min(hz, 20000.0f));
    if (prepared_) {
        highCut_.setCutoff(highCutHz_, static_cast<float>(sampleRate_));
    }
}

void IRLoader::setLowCut(float hz) {
    lowCutHz_ = std::max(20.0f, std::min(hz, 500.0f));
    if (prepared_) {
        lowCut_.setCutoff(lowCutHz_, static_cast<float>(sampleRate_));
    }
}

} // namespace openamp
