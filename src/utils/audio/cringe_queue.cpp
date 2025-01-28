/*
 * MIT License
 *
 * Copyright (c) 2023 @nulzo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "utils/audio/cringe_queue.h"

// bool CringeQueue::is_empty() const {
//     std::lock_guard<std::mutex> lock(mutex_);
//     return queue_.empty();
// }

// size_t CringeQueue::size() const {
//     std::lock_guard<std::mutex> lock(mutex_);
//     return queue_.size();
// }

// void CringeQueue::enqueue(const CringeSong& song) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     if (queue_.size() >= MAX_QUEUE_SIZE) {
//         throw std::runtime_error("Queue is full");
//     }
//     queue_.push(song);
// }

// std::optional<CringeSong> CringeQueue::dequeue() {
//     std::lock_guard<std::mutex> lock(mutex_);
//     if (queue_.empty()) {
//         return std::nullopt;
//     }
//     CringeSong song = std::move(queue_.front());
//     queue_.pop();
//     return song;
// }

// std::optional<std::string> CringeQueue::peek() const {
//     std::lock_guard<std::mutex> lock(mutex_);
//     if (queue_.empty()) {
//         return std::nullopt;
//     }
//     return queue_.front().title;
// }

// std::vector<CringeSong> CringeQueue::get_queue() const {
//     std::lock_guard<std::mutex> lock(mutex_);
//     std::vector<CringeSong> songs;
//     songs.reserve(queue_.size());
    
//     auto temp = queue_;
//     while (!temp.empty()) {
//         songs.push_back(std::move(temp.front()));
//         temp.pop();
//     }
    
//     return songs;
// }

// void CringeQueue::clear() {
//     std::lock_guard<std::mutex> lock(mutex_);
//     std::queue<CringeSong>().swap(queue_);
// }