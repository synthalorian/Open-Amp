#pragma once

// Lock-free single-producer single-consumer queue
// For passing data between audio thread and UI thread

#include <atomic>
#include <cstdint>
#include <memory>

namespace openamp {

template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity) 
        : capacity_(capacity)
        , buffer_(new std::atomic<T>[capacity])
        , head_(0)
        , tail_(0)
    {
        for (size_t i = 0; i < capacity; ++i) {
            buffer_[i].store(T{}, std::memory_order_relaxed);
        }
    }
    
    ~LockFreeQueue() = default;
    
    // Push an item (producer thread only)
    bool push(const T& item) {
        size_t currentHead = head_.load(std::memory_order_relaxed);
        size_t nextHead = (currentHead + 1) % capacity_;
        
        if (nextHead == tail_.load(std::memory_order_acquire)) {
            // Queue is full
            return false;
        }
        
        buffer_[currentHead].store(item, std::memory_order_relaxed);
        head_.store(nextHead, std::memory_order_release);
        return true;
    }
    
    // Push an item, overwrite oldest if full
    void push_overwrite(const T& item) {
        size_t currentHead = head_.load(std::memory_order_relaxed);
        size_t nextHead = (currentHead + 1) % capacity_;
        
        if (nextHead == tail_.load(std::memory_order_acquire)) {
            // Queue is full, advance tail to overwrite
            tail_.store((tail_.load(std::memory_order_relaxed) + 1) % capacity_, 
                       std::memory_order_release);
        }
        
        buffer_[currentHead].store(item, std::memory_order_relaxed);
        head_.store(nextHead, std::memory_order_release);
    }
    
    // Pop an item (consumer thread only)
    bool pop(T& item) {
        size_t currentTail = tail_.load(std::memory_order_relaxed);
        
        if (currentTail == head_.load(std::memory_order_acquire)) {
            // Queue is empty
            return false;
        }
        
        item = buffer_[currentTail].load(std::memory_order_relaxed);
        tail_.store((currentTail + 1) % capacity_, std::memory_order_release);
        return true;
    }
    
    // Peek at the front item without removing
    bool peek(T& item) const {
        size_t currentTail = tail_.load(std::memory_order_relaxed);
        
        if (currentTail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        
        item = buffer_[currentTail].load(std::memory_order_relaxed);
        return true;
    }
    
    // Check if empty
    bool empty() const {
        return head_.load(std::memory_order_acquire) == 
               tail_.load(std::memory_order_acquire);
    }
    
    // Check if full
    bool full() const {
        size_t nextHead = (head_.load(std::memory_order_relaxed) + 1) % capacity_;
        return nextHead == tail_.load(std::memory_order_acquire);
    }
    
    // Get number of items in queue
    size_t size() const {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return (h >= t) ? (h - t) : (capacity_ - t + h);
    }
    
    // Get capacity
    size_t capacity() const { return capacity_ - 1; }
    
private:
    const size_t capacity_;
    std::unique_ptr<std::atomic<T>[]> buffer_;
    alignas(64) std::atomic<size_t> head_;  // Cache line aligned
    alignas(64) std::atomic<size_t> tail_;
};

// Specialization for audio sample blocks
class AudioBlockQueue {
public:
    struct Block {
        float* data;
        uint32_t numFrames;
        uint32_t channels;
        uint64_t timestamp;
    };
    
    explicit AudioBlockQueue(size_t numBlocks = 4)
        : queue_(numBlocks)
    {}
    
    bool pushBlock(const float* data, uint32_t numFrames, uint32_t channels, uint64_t timestamp) {
        Block block;
        block.data = const_cast<float*>(data);
        block.numFrames = numFrames;
        block.channels = channels;
        block.timestamp = timestamp;
        return queue_.push(block);
    }
    
    bool popBlock(Block& block) {
        return queue_.pop(block);
    }
    
    bool empty() const { return queue_.empty(); }
    
private:
    LockFreeQueue<Block> queue_;
};

// Meter data for UI updates
struct MeterData {
    float inputLeftDb;
    float inputRightDb;
    float outputLeftDb;
    float outputRightDb;
    float inputPeakLeftDb;
    float inputPeakRightDb;
    float outputPeakLeftDb;
    float outputPeakRightDb;
    uint64_t timestamp;
};

using MeterQueue = LockFreeQueue<MeterData>;

} // namespace openamp
