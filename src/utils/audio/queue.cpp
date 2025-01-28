#include "utils/audio/queue.h"
#include <algorithm>
#include <random>

namespace cringe {

void QueueManager::add(std::shared_ptr<Track> track) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push_back(track);
}

void QueueManager::add_front(std::shared_ptr<Track> track) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push_front(track);
}

std::optional<std::shared_ptr<Track>> QueueManager::pop() {
    std::lock_guard<std::mutex> lock(mutex);
    if (queue.empty()) {
        return std::nullopt;
    }
    
    auto track = queue.front();
    queue.pop_front();
    return track;
}

bool QueueManager::remove(size_t index) {
    std::lock_guard<std::mutex> lock(mutex);
    if (index >= queue.size()) {
        return false;
    }
    
    queue.erase(queue.begin() + index);
    return true;
}

void QueueManager::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    queue.clear();
}

void QueueManager::add_to_history(std::shared_ptr<Track> track) {
    std::lock_guard<std::mutex> lock(mutex);
    history.push_front(track);
    
    while (history.size() > max_history_size) {
        history.pop_back();
    }
}

std::vector<std::shared_ptr<Track>> QueueManager::get_history() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::vector<std::shared_ptr<Track>>(history.begin(), history.end());
}

void QueueManager::clear_history() {
    std::lock_guard<std::mutex> lock(mutex);
    history.clear();
}

size_t QueueManager::size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
}

bool QueueManager::empty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
}

std::vector<std::shared_ptr<Track>> QueueManager::get_tracks() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::vector<std::shared_ptr<Track>>(queue.begin(), queue.end());
}

void QueueManager::shuffle() {
    std::lock_guard<std::mutex> lock(mutex);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(queue.begin(), queue.end(), g);
}

} // namespace cringe