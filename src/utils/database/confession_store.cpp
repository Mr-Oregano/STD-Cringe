#include "utils/database/confession_store.h"

ConfessionStore::ConfessionStore(CringeDB& database) : db(database) {
    db.execute(
        "CREATE TABLE IF NOT EXISTS confessions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "guild_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "image_url TEXT,"
        "created_at INTEGER NOT NULL,"
        "guild_confession_number INTEGER NOT NULL" 
        ")"
    );
    
    // Create index for guild_id for faster lookups
    db.createIndex("idx_confessions_guild", "confessions", "guild_id");
}

size_t ConfessionStore::getNextGuildConfessionNumber(dpp::snowflake guild_id) {
    auto results = db.query(
        "SELECT MAX(guild_confession_number) FROM confessions WHERE guild_id = ?",
        {std::to_string(guild_id)}
    );
    
    if (results.empty() || results[0][0].empty()) {
        return 1;
    }
    
    return std::stoull(results[0][0]) + 1;
}

dpp::snowflake ConfessionStore::createConfession(
    dpp::snowflake guild_id,
    dpp::snowflake channel_id,
    const std::string& content,
    const std::string& image_url
) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    size_t guild_confession_number = getNextGuildConfessionNumber(guild_id);
    
    db.execute(
        "INSERT INTO confessions "
        "(guild_id, channel_id, content, image_url, created_at, guild_confession_number) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {
            std::to_string(guild_id),
            std::to_string(channel_id),
            content,
            image_url,
            std::to_string(timestamp),
            std::to_string(guild_confession_number)
        }
    );
    
    auto result = db.query("SELECT last_insert_rowid()");
    return std::stoull(result[0][0]);
}

std::optional<Confession> ConfessionStore::getConfession(dpp::snowflake id) {
    auto results = db.query(
        "SELECT id, guild_id, channel_id, content, image_url, created_at, guild_confession_number "
        "FROM confessions WHERE id = ?",
        {std::to_string(id)}
    );
    
    if (results.empty()) {
        return std::nullopt;
    }
    
    const auto& row = results[0];
    return Confession{
        std::stoull(row[0]),  // id
        std::stoull(row[1]),  // guild_id
        std::stoull(row[2]),  // channel_id
        row[3],               // content
        row[4],               // image_url
        std::chrono::system_clock::time_point(std::chrono::milliseconds(std::stoull(row[5]))), // created_at
        std::stoull(row[6])   // guild_confession_number
    };
}

std::vector<Confession> ConfessionStore::getGuildConfessions(dpp::snowflake guild_id) {
    auto results = db.query(
        "SELECT id, guild_id, channel_id, content, image_url, created_at, guild_confession_number "
        "FROM confessions WHERE guild_id = ? ORDER BY created_at DESC",
        {std::to_string(guild_id)}
    );
    
    std::vector<Confession> confessions;
    for (const auto& row : results) {
        confessions.push_back({
            std::stoull(row[0]),  // id
            std::stoull(row[1]),  // guild_id
            std::stoull(row[2]),  // channel_id
            row[3],               // content
            row[4],               // image_url
            std::chrono::system_clock::time_point(std::chrono::milliseconds(std::stoull(row[5]))), // created_at
            std::stoull(row[6])   // guild_confession_number
        });
    }
    
    return confessions;
}