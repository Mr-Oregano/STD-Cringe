#ifndef CRINGE_CHALLENGE_STORE_H
#define CRINGE_CHALLENGE_STORE_H

#include <optional>
#include <sqlite3.h>
#include <vector>

#include "commands/chat/challenge_command.h"
#include "cringe_database.h"

class CringeChallengeStore {
public:
	explicit CringeChallengeStore(CringeDB& database);

    // Initialize the database
    void initTables();

    // Get all challenges
    std::vector<Challenge> getAllChallenges();

    // Get a specific challenge
    std::optional<Challenge> getChallenge(dpp::snowflake challenge_id);

    // Submit an answer to a challenge
    bool submitAnswer(dpp::snowflake user_id, dpp::snowflake challenge_id, int part, const std::string& answer);

    // Get the leaderboard for a challenge
    std::vector<std::pair<dpp::snowflake, int>> getLeaderboard(bool global, dpp::snowflake guild_id);

    // Get the stats for a user
    std::tuple<int, int, int> getUserStats(dpp::snowflake user_id);

    // Check if a part is completed
    bool isPartCompleted(dpp::snowflake user_id, dpp::snowflake challenge_id, int part);
	int getCurrentPart(dpp::snowflake user_id, dpp::snowflake challenge_id);


private:
    CringeDB& db;
};

#endif