#ifndef CRINGE_CONSPIRACY_STORE_H
#define CRINGE_CONSPIRACY_STORE_H

#include "utils/database/cringe_database.h"
#include <dpp/dpp.h>
#include <optional>
#include <vector>
#include <string>
#include <chrono>

struct ConspiracyTheory {
    dpp::snowflake id;
    dpp::snowflake guild_id;
    dpp::snowflake creator_id;
    std::string title;
    std::string content;
    std::vector<dpp::snowflake> involved_users;
    std::vector<dpp::snowflake> evidence_messages;
    int complexity_level;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

class ConspiracyStore {
private:
    CringeDB& db;
    void initTables();

public:
    explicit ConspiracyStore(CringeDB& database);

    // Create a new conspiracy theory
    dpp::snowflake createConspiracy(
        dpp::snowflake guild_id,
        dpp::snowflake creator_id,
        const std::string& title,
        const std::string& content,
        const std::vector<dpp::snowflake>& involved_users,
        const std::vector<dpp::snowflake>& evidence_messages
    );

    // Get a conspiracy theory by ID
    std::optional<ConspiracyTheory> getConspiracy(dpp::snowflake id);

    // Get all conspiracies for a guild
    std::vector<ConspiracyTheory> getGuildConspiracies(dpp::snowflake guild_id);

    // Update a conspiracy theory
    bool updateConspiracy(const ConspiracyTheory& theory);

    // Delete a conspiracy theory
    bool deleteConspiracy(dpp::snowflake id);

    // Increment complexity level
    bool incrementComplexity(dpp::snowflake id);
};

#endif