#include "recorder.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <filesystem>

namespace openamp {

Recorder::Recorder() {
    sampleBuffer_.reserve(kBufferSize);
}

Recorder::~Recorder() {
    if (state_ == State::Recording) {
        stopRecording();
    }
}

void Recorder::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    sampleBuffer_.clear();
    sampleBuffer_.reserve(kBufferSize);
}

void Recorder::reset() {
    if (state_ == State::Recording) {
        stopRecording();
    }
    recordedSamples_ = 0;
    sampleBuffer_.clear();
}

void Recorder::process(AudioBuffer& buffer) {
    if (bypass_ || state_ != State::Recording) return;
    
    writeSamples(buffer.data, buffer.numFrames);
    
    // Fire progress callback
    if (progressCallback_) {
        progressCallback_(getRecordingInfo());
    }
}

void Recorder::startRecording(const std::string& filename) {
    currentFilename_ = filename;
    recordedSamples_ = 0;
    sampleBuffer_.clear();
    
    // Create/truncate file and write WAV header
    writeWavHeader();
    
    changeState(State::Recording);
}

void Recorder::stopRecording() {
    if (state_ != State::Recording) return;
    
    // Flush remaining samples
    if (!sampleBuffer_.empty()) {
        // Write remaining samples to file
    }
    
    // Update WAV header with final size
    updateWavHeader();
    
    changeState(State::Stopped);
    currentFilename_.clear();
}

void Recorder::pauseRecording() {
    if (state_ == State::Recording) {
        changeState(State::Paused);
    }
}

void Recorder::resumeRecording() {
    if (state_ == State::Paused) {
        changeState(State::Recording);
    }
}

Recorder::RecordingInfo Recorder::getRecordingInfo() const {
    RecordingInfo info;
    info.totalSamples = recordedSamples_;
    info.durationSeconds = static_cast<float>(recordedSamples_) / static_cast<float>(sampleRate_);
    info.filename = currentFilename_;
    info.fileSize = recordedSamples_ * sizeof(int16_t) * 1 + 44;  // Mono WAV
    return info;
}

void Recorder::setBypass(bool bypass) {
    bypass_ = bypass;
}

void Recorder::setFileFormat(int format) {
    fileFormat_ = std::clamp(format, 0, 2);
}

void Recorder::setBitDepth(int bits) {
    bitDepth_ = (bits == 16 || bits == 24 || bits == 32) ? bits : 24;
}

void Recorder::setMaxFileSize(uint64_t bytes) {
    maxFileSize_ = bytes;
}

void Recorder::setStateCallback(StateCallback callback) {
    stateCallback_ = callback;
}

void Recorder::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
}

std::vector<std::string> Recorder::listRecordings() const {
    std::vector<std::string> recordings;
    // Implementation would scan recordings directory
    return recordings;
}

bool Recorder::deleteRecording(const std::string& filename) {
    try {
        return std::filesystem::remove(filename);
    } catch (...) {
        return false;
    }
}

std::string Recorder::getRecordingPath(const std::string& filename) const {
    return filename;
}

bool Recorder::exportToMP3(const std::string& wavPath, const std::string& mp3Path) {
    // Would need lame or similar library
    return false;
}

void Recorder::writeWavHeader() {
    // Standard WAV header (44 bytes)
    // This is a placeholder - real implementation would write to file
    uint8_t header[44] = {0};
    
    // RIFF chunk
    std::memcpy(header + 0, "RIFF", 4);
    uint32_t fileSize = 0;  // Will be updated later
    std::memcpy(header + 4, &fileSize, 4);
    std::memcpy(header + 8, "WAVE", 4);
    
    // fmt chunk
    std::memcpy(header + 12, "fmt ", 4);
    uint32_t fmtSize = 16;
    std::memcpy(header + 16, &fmtSize, 4);
    uint16_t audioFormat = 1;  // PCM
    std::memcpy(header + 20, &audioFormat, 2);
    uint16_t numChannels = 1;  // Mono
    std::memcpy(header + 22, &numChannels, 2);
    uint32_t sampleRate = static_cast<uint32_t>(sampleRate_);
    std::memcpy(header + 24, &sampleRate, 4);
    uint32_t byteRate = static_cast<uint32_t>(sampleRate_ * numChannels * bitDepth_ / 8);
    std::memcpy(header + 28, &byteRate, 4);
    uint16_t blockAlign = numChannels * bitDepth_ / 8;
    std::memcpy(header + 32, &blockAlign, 2);
    uint16_t bitsPerSample = bitDepth_;
    std::memcpy(header + 34, &bitsPerSample, 2);
    
    // data chunk
    std::memcpy(header + 36, "data", 4);
    uint32_t dataSize = 0;  // Will be updated later
    std::memcpy(header + 40, &dataSize, 4);
    
    // Would write header to file here
}

void Recorder::updateWavHeader() {
    // Update the file size fields in the WAV header
    uint32_t dataSize = static_cast<uint32_t>(recordedSamples_ * sizeof(int16_t));
    uint32_t fileSize = dataSize + 36;
    
    // Would seek to positions 4 and 40 and update values
}

void Recorder::writeSamples(const float* data, uint32_t numFrames) {
    // Convert float to int16 and buffer
    for (uint32_t i = 0; i < numFrames; ++i) {
        float sample = std::clamp(data[i], -1.0f, 1.0f);
        int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
        sampleBuffer_.push_back(intSample);
        recordedSamples_++;
    }
    
    // Flush to file when buffer is full
    if (sampleBuffer_.size() >= kBufferSize) {
        // Would write sampleBuffer_ to file here
        sampleBuffer_.clear();
    }
    
    // Check max file size
    if (maxFileSize_ > 0) {
        uint64_t currentSize = recordedSamples_ * sizeof(int16_t) + 44;
        if (currentSize >= maxFileSize_) {
            stopRecording();
        }
    }
}

void Recorder::changeState(State newState) {
    state_ = newState;
    if (stateCallback_) {
        stateCallback_(state_);
    }
}

} // namespace openamp
