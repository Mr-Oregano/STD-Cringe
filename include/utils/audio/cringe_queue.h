#ifndef CRINGE_CRINGE_QUEUE_H
#define CRINGE_CRINGE_QUEUE_H

#include <deque>
#include <mutex>
#include <memory>
#include <optional>
#include "utils/audio/cringe_song.h"

class CringeQueue {
public:
    CringeQueue() = default;
    ~CringeQueue() = default;
    
    // Delete move operations since we have a mutex
    CringeQueue(CringeQueue&&) = delete;
    CringeQueue& operator=(CringeQueue&&) = delete;
    
    // Delete copy operations
    CringeQueue(const CringeQueue&) = delete;
    CringeQueue& operator=(const CringeQueue&) = delete;

    [[nodiscard]] bool is_empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void enqueue(const CringeSong& song) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= MAX_QUEUE_SIZE) {
            throw std::runtime_error("Queue is full");
        }
        queue_.push_back(song);
    }

    std::optional<CringeSong> dequeue() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        CringeSong song = queue_.front();
        queue_.pop_front();
        return song;
    }

    std::deque<CringeSong> get_queue() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }

    static constexpr size_t MAX_QUEUE_SIZE = 100;

private:
    mutable std::mutex mutex_;
    std::deque<CringeSong> queue_;
};

#endif // CRINGE_CRINGE_QUEUE_H