#pragma once

#include "openamp/plugin_interface.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace openamp {

class Recorder : public AudioProcessor {
public:
    enum class State {
        Stopped,
        Recording,
        Paused
    };
    
    struct RecordingInfo {
        uint64_t totalSamples;
        float durationSeconds;
        uint64_t fileSize;
        std::string filename;
    };
    
    using StateCallback = std::function<void(State)>;
    using ProgressCallback = std::function<void(const RecordingInfo&)>;
    
    Recorder();
    ~Recorder();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Recorder"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Transport
    void startRecording(const std::string& filename);
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    
    State getState() const { return state_; }
    RecordingInfo getRecordingInfo() const;
    
    // Settings
    void setMaxFileSize(uint64_t bytes);  // 0 = unlimited
    uint64_t getMaxFileSize() const { return maxFileSize_; }
    
    void setFileFormat(int format);  // 0=WAV, 1=FLAC, 2=MP3
    int getFileFormat() const { return fileFormat_; }
    
    void setBitDepth(int bits);  // 16, 24, or 32
    int getBitDepth() const { return bitDepth_; }
    
    // Bypass (pass audio through without recording)
    void setBypass(bool bypass);
    bool isBypassed() const { return bypass_; }
    
    // Callbacks
    void setStateCallback(StateCallback callback);
    void setProgressCallback(ProgressCallback callback);
    
    // File management
    std::vector<std::string> listRecordings() const;
    bool deleteRecording(const std::string& filename);
    std::string getRecordingPath(const std::string& filename) const;
    
    // Export
    bool exportToMP3(const std::string& wavPath, const std::string& mp3Path);

private:
    double sampleRate_ = 48000.0;
    
    State state_ = State::Stopped;
    bool bypass_ = false;
    
    std::string currentFilename_;
    uint64_t recordedSamples_ = 0;
    uint64_t maxFileSize_ = 0;  // 0 = unlimited
    int fileFormat_ = 0;  // 0=WAV
    int bitDepth_ = 24;
    
    // WAV file buffer
    std::vector<int16_t> sampleBuffer_;
    std::vector<uint8_t> fileBuffer_;
    static constexpr int kBufferSize = 4096;
    
    // Callbacks
    StateCallback stateCallback_;
    ProgressCallback progressCallback_;
    
    void writeWavHeader();
    void updateWavHeader();
    void writeSamples(const float* data, uint32_t numFrames);
    void changeState(State newState);
};

} // namespace openamp
