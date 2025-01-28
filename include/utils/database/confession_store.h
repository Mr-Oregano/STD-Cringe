#ifndef CRINGE_CONFESSION_STORE_H
#define CRINGE_CONFESSION_STORE_H

#include <dpp/dpp.h>
#include <chrono>
#include <optional>
#include <string>
#include <vector>
#include "utils/database/cringe_database.h"

struct Confession {
    dpp::snowflake id;                    // Global unique ID
    dpp::snowflake guild_id;              // Discord server ID
    dpp::snowflake channel_id;            // Channel the confession was sent to
    std::string content;                  // The confession text
    std::string image_url;                // Optional image attachment
    std::chrono::system_clock::time_point created_at;  // When the confession was made
    size_t guild_confession_number;        // Server-specific confession number
};

class ConfessionStore {
public:
    explicit ConfessionStore(CringeDB& db);
    
    // Create a new confession and return its global ID
    dpp::snowflake createConfession(
        dpp::snowflake guild_id,
        dpp::snowflake channel_id,
        const std::string& content,
        const std::string& image_url = ""
    );
    
    // Get a confession by its global ID
    std::optional<Confession> getConfession(dpp::snowflake id);
    
    // Get all confessions for a specific guild
    std::vector<Confession> getGuildConfessions(dpp::snowflake guild_id);
    
    // Get the next confession number for a specific guild
    size_t getNextGuildConfessionNumber(dpp::snowflake guild_id);

private:
    CringeDB& db;
};

#endif //CRINGE_CONFESSION_STORE_H