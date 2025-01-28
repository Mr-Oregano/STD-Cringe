#ifndef CRINGE_RAGE_STORE_H
#define CRINGE_RAGE_STORE_H

#include <string>
#include <vector>
#include <chrono>
#include "utils/database/cringe_database.h"
#include <dpp/dpp.h>

struct RageInteraction {
    dpp::snowflake message_id;
    dpp::snowflake user_id;
    dpp::snowflake guild_id;
    std::string content;
    std::string reaction;
    double anger_score;
    std::chrono::system_clock::time_point timestamp;
    dpp::snowflake referenced_message_id;
    bool is_bot_message;               
};

class RageStore {
private:
    CringeDB& db;
    void initTables();

public:
    explicit RageStore(CringeDB& database);
    
    // Store operations
    void storeRageInteraction(const RageInteraction& interaction);
    void storeRageBait(dpp::snowflake message_id, dpp::snowflake channel_id, const std::string& content);
    
    // Retrieval operations
    std::vector<RageInteraction> getRecentInteractions(dpp::snowflake user_id, size_t limit = 10);
    std::vector<RageInteraction> getGuildInteractions(dpp::snowflake guild_id, size_t limit = 10);
    double getAngerScore(dpp::snowflake user_id);
    double getGuildAngerScore(dpp::snowflake guild_id);
    
    // Cleanup operations
    void cleanupOldInteractions(std::chrono::hours age = std::chrono::hours(24 * 7));

    std::vector<RageInteraction> getConversationThread(
        dpp::snowflake original_bait_id,
        dpp::snowflake user_id
    );
    
    dpp::snowflake getOriginalBaitId(dpp::snowflake message_id);
    bool isRageBait(dpp::snowflake message_id);
};

#endif