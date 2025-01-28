#pragma once
#include "include/utils/audio/track.h"
#include <deque>
#include <mutex>
#include <optional>

namespace cringe {

class QueueManager {
private:
    std::deque<std::shared_ptr<Track>> queue;
    std::deque<std::shared_ptr<Track>> history;
    mutable std::mutex mutex;
    size_t max_history_size{100};

public:
    QueueManager() = default;
    
    // Queue operations
    void add(std::shared_ptr<Track> track);
    void add_front(std::shared_ptr<Track> track);
    std::optional<std::shared_ptr<Track>> pop();
    bool remove(size_t index);
    void clear();
    
    // History operations
    void add_to_history(std::shared_ptr<Track> track);
    std::vector<std::shared_ptr<Track>> get_history() const;
    void clear_history();
    
    // Utility
    size_t size() const;
    bool empty() const;
    std::vector<std::shared_ptr<Track>> get_tracks() const;
    void shuffle();
};

} // namespace cringe