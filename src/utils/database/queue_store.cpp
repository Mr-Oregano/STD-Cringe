#include "utils/database/queue_store.h"

QueueStore::QueueStore(CringeDB& database) : db(database) {
    db.execute(
        "CREATE TABLE IF NOT EXISTS queue ("
        "guild_id TEXT NOT NULL,"
        "position INTEGER NOT NULL,"
        "song_data TEXT NOT NULL,"
        "PRIMARY KEY (guild_id, position))");
}

void QueueStore::addSong(dpp::snowflake guild_id, const CringeSong& song) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t position = getQueueSize(guild_id);
    
    db.execute(
        "INSERT INTO queue (guild_id, position, song_data) VALUES (?, ?, ?)",
        {std::to_string(guild_id), std::to_string(position), song.serialize()});
}

std::optional<CringeSong> QueueStore::getNextSong(dpp::snowflake guild_id) {
    std::cout << "getNextSong called for guild: " << guild_id << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        auto results = db.query(
            "SELECT song_data FROM queue WHERE guild_id = ? ORDER BY position ASC LIMIT 1",
            {std::to_string(guild_id)});
        
        std::cout << "Query results size: " << results.size() << std::endl;
        
        if (results.empty() || results[0].empty()) {
            std::cout << "No results found in queue" << std::endl;
            return std::nullopt;
        }
        
        // Store the song data before modifying the queue
        auto song_data = results[0][0];
        
        // Remove the song from queue
        db.execute(
            "DELETE FROM queue WHERE guild_id = ? AND position = 0",
            {std::to_string(guild_id)});
            
        // Reorder remaining songs
        db.execute(
            "UPDATE queue SET position = position - 1 WHERE guild_id = ?",
            {std::to_string(guild_id)});
        
        // Deserialize and return
        auto song = CringeSong::deserialize(song_data);
        std::cout << "Successfully retrieved and removed song from queue" << std::endl;
        return song;
    } catch (const std::exception& e) {
        std::cerr << "Error in getNextSong: " << e.what() << std::endl;
        throw;
    }
}

void QueueStore::removeSong(dpp::snowflake guild_id, size_t position) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    db.execute(
        "DELETE FROM queue WHERE guild_id = ? AND position = ?",
        {std::to_string(guild_id), std::to_string(position)});
        
    // Reorder remaining songs
    db.execute(
        "UPDATE queue SET position = position - 1 WHERE guild_id = ? AND position > ?",
        {std::to_string(guild_id), std::to_string(position)});
}

std::vector<CringeSong> QueueStore::getQueue(dpp::snowflake guild_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto results = db.query(
        "SELECT song_data FROM queue WHERE guild_id = ? ORDER BY position ASC",
        {std::to_string(guild_id)});
        
    std::vector<CringeSong> songs;
    for (const auto& row : results) {
        if (!row.empty()) {
            songs.push_back(CringeSong::deserialize(row[0]));
        }
    }
    return songs;
}

size_t QueueStore::getQueueSize(dpp::snowflake guild_id) {
    auto results = db.query(
        "SELECT COUNT(*) FROM queue WHERE guild_id = ?",
        {std::to_string(guild_id)});
        
    return results.empty() || results[0].empty() ? 0 : std::stoull(results[0][0]);
}

void QueueStore::clearQueue(dpp::snowflake guild_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    db.execute("DELETE FROM queue WHERE guild_id = ?", {std::to_string(guild_id)});
}