#ifndef CRINGE_QUEUE_STORE_H
#define CRINGE_QUEUE_STORE_H

#include <memory>
#include <optional>
#include <vector>
#include "utils/audio/cringe_song.h"
#include "utils/database/cringe_database.h"

class QueueStore {
public:
    explicit QueueStore(CringeDB& db);
    
    // Queue operations
    void addSong(dpp::snowflake guild_id, const CringeSong& song);
    std::optional<CringeSong> getNextSong(dpp::snowflake guild_id);
    void removeSong(dpp::snowflake guild_id, size_t position);
    std::vector<CringeSong> getQueue(dpp::snowflake guild_id);
    void clearQueue(dpp::snowflake guild_id);
    size_t getQueueSize(dpp::snowflake guild_id);
    template<typename Callback>
    void withNextSong(dpp::snowflake guild_id, Callback&& callback) {
        if (auto next = getNextSong(guild_id)) {
            callback(std::move(*next));
        }
    }
    std::mutex& getMutex() { return mutex_; }

    
private:
    CringeDB& db;
    mutable std::mutex mutex_;
};

#endif