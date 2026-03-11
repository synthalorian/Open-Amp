#include "looper.h"
#include <cmath>
#include <algorithm>

namespace openamp {

Looper::Looper() = default;
Looper::~Looper() = default;

void Looper::prepare(double sampleRate, uint32_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    
    // Pre-allocate loop buffer for max duration
    size_t maxSize = static_cast<size_t>(maxDurationSeconds_ * sampleRate_);
    loopBuffer_.reserve(maxSize);
    loopBuffer_.clear();
    
    loopLength_ = 0;
    playPosition_ = 0;
    numOverdubs_ = 0;
    state_ = State::Stopped;
}

void Looper::reset() {
    loopBuffer_.clear();
    loopLength_ = 0;
    playPosition_ = 0;
    numOverdubs_ = 0;
    state_ = State::Stopped;
    undoStack_.clear();
    redoStack_.clear();
}

void Looper::process(AudioBuffer& buffer) {
    if (bypass_) return;
    
    float* data = buffer.data;
    uint32_t numFrames = buffer.numFrames;
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        float input = data[i] * inputLevel_;
        float output = 0.0f;
        
        switch (state_) {
            case State::Recording: {
                // Record input to buffer
                if (loopBuffer_.size() < loopBuffer_.capacity()) {
                    loopBuffer_.push_back(input);
                    loopLength_ = loopBuffer_.size();
                }
                // Pass through input
                output = input;
                break;
            }
            
            case State::Playing: {
                if (loopLength_ > 0) {
                    // Play from buffer
                    output = loopBuffer_[playPosition_] * playbackLevel_;
                    playPosition_ = (playPosition_ + 1) % loopLength_;
                }
                // Mix with dry input
                data[i] = input * (1.0f - mix_) + (input + output) * mix_;
                break;
            }
            
            case State::Overdubbing: {
                if (loopLength_ > 0) {
                    // Play existing loop
                    float loopSample = loopBuffer_[playPosition_];
                    
                    // Record new layer with feedback
                    loopBuffer_[playPosition_] = loopSample * feedback_ + input * overdubLevel_;
                    
                    output = loopBuffer_[playPosition_] * playbackLevel_;
                    playPosition_ = (playPosition_ + 1) % loopLength_;
                }
                // Mix with dry input
                data[i] = input * (1.0f - mix_) + (input + output) * mix_;
                break;
            }
            
            case State::Paused: {
                // Just pass through input
                data[i] = input;
                break;
            }
            
            case State::Stopped: {
                // Pass through input
                data[i] = input;
                break;
            }
        }
    }
}

void Looper::record() {
    if (state_ == State::Stopped) {
        // Start new recording
        saveToUndo();
        loopBuffer_.clear();
        loopLength_ = 0;
        playPosition_ = 0;
        numOverdubs_ = 0;
        changeState(State::Recording);
    } else if (state_ == State::Recording) {
        // Stop recording, start playback
        if (quantize_) {
            loopLength_ = getQuantizedLength();
            loopBuffer_.resize(loopLength_);
        }
        numOverdubs_ = 0;
        changeState(State::Playing);
    } else if (state_ == State::Playing) {
        // Start overdubbing
        saveToUndo();
        changeState(State::Overdubbing);
    } else if (state_ == State::Overdubbing) {
        // Stop overdubbing, continue playback
        numOverdubs_++;
        changeState(State::Playing);
    }
}

void Looper::play() {
    if (state_ == State::Stopped && loopLength_ > 0) {
        playPosition_ = 0;
        changeState(State::Playing);
    } else if (state_ == State::Paused) {
        changeState(State::Playing);
    }
}

void Looper::stop() {
    changeState(State::Stopped);
}

void Looper::pause() {
    if (state_ == State::Playing || state_ == State::Recording || state_ == State::Overdubbing) {
        changeState(State::Paused);
    }
}

void Looper::overdub() {
    if (state_ == State::Playing && loopLength_ > 0) {
        saveToUndo();
        changeState(State::Overdubbing);
    } else if (state_ == State::Overdubbing) {
        numOverdubs_++;
        changeState(State::Playing);
    }
}

void Looper::clear() {
    saveToUndo();
    loopBuffer_.clear();
    loopLength_ = 0;
    playPosition_ = 0;
    numOverdubs_ = 0;
    changeState(State::Stopped);
}

Looper::LoopInfo Looper::getLoopInfo() const {
    LoopInfo info;
    info.totalSamples = loopLength_;
    info.durationSeconds = static_cast<float>(loopLength_) / static_cast<float>(sampleRate_);
    info.numOverdubs = numOverdubs_;
    info.currentSample = playPosition_;
    info.position = loopLength_ > 0 ? static_cast<float>(playPosition_) / static_cast<float>(loopLength_) : 0.0f;
    return info;
}

void Looper::setMaxDuration(float seconds) {
    maxDurationSeconds_ = std::clamp(seconds, 1.0f, 300.0f);  // 1s to 5min
    size_t maxSize = static_cast<size_t>(maxDurationSeconds_ * sampleRate_);
    loopBuffer_.reserve(maxSize);
}

void Looper::setPlaybackLevel(float level) {
    playbackLevel_ = std::clamp(level, 0.0f, 2.0f);
}

void Looper::setInputLevel(float level) {
    inputLevel_ = std::clamp(level, 0.0f, 2.0f);
}

void Looper::setOverdubLevel(float level) {
    overdubLevel_ = std::clamp(level, 0.0f, 1.0f);
}

void Looper::setFeedback(float fb) {
    feedback_ = std::clamp(fb, 0.0f, 1.0f);
}

void Looper::setMix(float mix) {
    mix_ = std::clamp(mix, 0.0f, 1.0f);
}

void Looper::setBypass(bool bypass) {
    bypass_ = bypass;
}

void Looper::setQuantize(bool quantize) {
    quantize_ = quantize;
}

void Looper::setTempo(float bpm) {
    tempo_ = std::clamp(bpm, 20.0f, 300.0f);
}

void Looper::setStateCallback(StateCallback callback) {
    stateCallback_ = callback;
}

bool Looper::canUndo() const {
    return !undoStack_.empty();
}

bool Looper::canRedo() const {
    return !redoStack_.empty();
}

void Looper::undo() {
    if (!undoStack_.empty()) {
        redoStack_.push_back(std::move(loopBuffer_));
        loopBuffer_ = std::move(undoStack_.back());
        undoStack_.pop_back();
        loopLength_ = loopBuffer_.size();
        numOverdubs_ = std::max(0, numOverdubs_ - 1);
    }
}

void Looper::redo() {
    if (!redoStack_.empty()) {
        undoStack_.push_back(std::move(loopBuffer_));
        loopBuffer_ = std::move(redoStack_.back());
        redoStack_.pop_back();
        loopLength_ = loopBuffer_.size();
        numOverdubs_++;
    }
}

void Looper::saveToUndo() {
    undoStack_.push_back(loopBuffer_);
    if (undoStack_.size() > kMaxUndoLevels) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
}

void Looper::changeState(State newState) {
    state_ = newState;
    if (stateCallback_) {
        stateCallback_(state_);
    }
}

uint64_t Looper::getQuantizedLength() const {
    if (!quantize_ || tempo_ <= 0) return loopLength_;
    
    // Quantize to nearest beat
    float samplesPerBeat = (60.0f / tempo_) * static_cast<float>(sampleRate_);
    float beats = static_cast<float>(loopLength_) / samplesPerBeat;
    int roundedBeats = std::max(1, static_cast<int>(std::round(beats)));
    
    return static_cast<uint64_t>(roundedBeats * samplesPerBeat);
}

} // namespace openamp
