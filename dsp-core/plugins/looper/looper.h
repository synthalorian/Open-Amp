#pragma once

#include "openamp/plugin_interface.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace openamp {

class Looper : public AudioProcessor {
public:
    enum class State {
        Stopped,
        Recording,
        Playing,
        Overdubbing,
        Paused
    };
    
    struct LoopInfo {
        uint64_t totalSamples;      // Total samples in loop
        float durationSeconds;      // Duration in seconds
        int numOverdubs;            // Number of overdub layers
        uint64_t currentSample;     // Current playback position
        float position;             // Position 0-1
    };
    
    using StateCallback = std::function<void(State)>;
    
    Looper();
    ~Looper();
    
    void prepare(double sampleRate, uint32_t maxBlockSize) override;
    void process(AudioBuffer& buffer) override;
    void reset() override;
    
    std::string getName() const override { return "Looper"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    // Transport controls
    void record();                 // Start recording (or overdub if loop exists)
    void play();                   // Start playback
    void stop();                   // Stop and clear
    void pause();                  // Pause playback/recording
    void overdub();                // Start overdubbing
    void clear();                  // Clear the loop
    
    // State
    State getState() const { return state_; }
    LoopInfo getLoopInfo() const;
    
    // Settings
    void setMaxDuration(float seconds);  // Maximum loop duration
    float getMaxDuration() const { return maxDurationSeconds_; }
    
    void setPlaybackLevel(float level);  // 0 to 1
    float getPlaybackLevel() const { return playbackLevel_; }
    
    void setInputLevel(float level);     // 0 to 1
    float getInputLevel() const { return inputLevel_; }
    
    void setOverdubLevel(float level);   // 0 to 1
    float getOverdubLevel() const { return overdubLevel_; }
    
    void setFeedback(float fb);          // 0 to 1 (for decay on overdubs)
    float getFeedback() const { return feedback_; }
    
    void setMix(float mix);              // 0 to 1 (dry/wet)
    float getMix() const { return mix_; }
    
    void setBypass(bool bypass);
    bool isBypassed() const { return bypass_; }
    
    // Quantization
    void setQuantize(bool quantize);
    bool isQuantized() const { return quantize_; }
    
    // Tempo sync (for quantization)
    void setTempo(float bpm);
    float getTempo() const { return tempo_; }
    
    // State change callback
    void setStateCallback(StateCallback callback);
    
    // Undo/Redo
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();

private:
    double sampleRate_ = 48000.0;
    
    State state_ = State::Stopped;
    bool bypass_ = false;
    
    // Loop buffer
    std::vector<float> loopBuffer_;
    uint64_t loopLength_ = 0;
    uint64_t playPosition_ = 0;
    int numOverdubs_ = 0;
    
    // Settings
    float maxDurationSeconds_ = 60.0f;  // 1 minute max
    float playbackLevel_ = 1.0f;
    float inputLevel_ = 1.0f;
    float overdubLevel_ = 0.5f;
    float feedback_ = 1.0f;
    float mix_ = 0.5f;
    
    // Quantization
    bool quantize_ = false;
    float tempo_ = 120.0f;
    uint64_t quantizePosition_ = 0;
    
    // Undo stack
    std::vector<std::vector<float>> undoStack_;
    std::vector<std::vector<float>> redoStack_;
    static constexpr int kMaxUndoLevels = 10;
    
    // Callback
    StateCallback stateCallback_;
    
    void saveToUndo();
    void changeState(State newState);
    uint64_t getQuantizedLength() const;
};

} // namespace openamp
