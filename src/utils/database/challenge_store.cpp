#include "utils/database/challenge_store.h"
#include <fmt/format.h>
#include <chrono>

CringeChallengeStore::CringeChallengeStore(CringeDB& database) : db(database) {
    initTables();
}

void CringeChallengeStore::initTables() {
    // Create challenges table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS challenges (
            challenge_id INTEGER PRIMARY KEY,
            title TEXT NOT NULL,
            description TEXT NOT NULL,
            created_at INTEGER DEFAULT (strftime('%s', 'now'))
        )
    )");

    // Create challenge parts table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS challenge_parts (
            part_id INTEGER PRIMARY KEY,
            challenge_id INTEGER NOT NULL,
            part_number INTEGER NOT NULL,
            description TEXT NOT NULL,
            answer TEXT NOT NULL,
            points INTEGER NOT NULL,
            FOREIGN KEY (challenge_id) REFERENCES challenges(challenge_id) ON DELETE CASCADE,
            UNIQUE (challenge_id, part_number)
        )
    )");

    // Create user progress table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS user_progress (
            user_id INTEGER NOT NULL,
            challenge_id INTEGER NOT NULL,
            part_number INTEGER NOT NULL,
            points_earned INTEGER NOT NULL,
            completed_at INTEGER DEFAULT (strftime('%s', 'now')),
            PRIMARY KEY (user_id, challenge_id, part_number),
            FOREIGN KEY (challenge_id) REFERENCES challenges(challenge_id) ON DELETE CASCADE
        )
    )");

    // Create helpful indexes
    db.createIndex("idx_challenge_parts", "challenge_parts", "challenge_id, part_number");
    db.createIndex("idx_user_progress", "user_progress", "user_id");
    db.createIndex("idx_challenge_completion", "user_progress", "challenge_id, user_id");
}

std::vector<Challenge> CringeChallengeStore::getAllChallenges() {
    auto results = db.query(R"(
        SELECT c.challenge_id, c.title, c.description, 
               cp.part_number, cp.description, cp.answer, cp.points
        FROM challenges c
        LEFT JOIN challenge_parts cp ON c.challenge_id = cp.challenge_id
        ORDER BY c.challenge_id, cp.part_number
    )");

    std::vector<Challenge> challenges;
    dpp::snowflake current_challenge_id = 0;
    Challenge* current_challenge = nullptr;

    for (const auto& row : results) {
        dpp::snowflake challenge_id = std::stoull(row[0]);
        
        if (challenge_id != current_challenge_id) {
            if (current_challenge) {
                challenges.push_back(*current_challenge);
            }
            challenges.emplace_back();
            current_challenge = &challenges.back();
            current_challenge->challenge_id = challenge_id;
            current_challenge->title = row[1];
            current_challenge->description = row[2];
            current_challenge_id = challenge_id;
        }
        
        if (!row[3].empty()) {
            ChallengePart part;
            part.part_number = std::stoi(row[3]);
            part.description = row[4];
            part.answer = row[5];
            part.points = std::stoi(row[6]);
            current_challenge->parts.push_back(part);
        }
    }

    if (current_challenge) {
        challenges.push_back(*current_challenge);
    }

    return challenges;
}

std::optional<Challenge> CringeChallengeStore::getChallenge(dpp::snowflake challenge_id) {
    auto results = db.query(R"(
        SELECT c.challenge_id, c.title, c.description, 
               cp.part_number, cp.description, cp.answer, cp.points
        FROM challenges c
        LEFT JOIN challenge_parts cp ON c.challenge_id = cp.challenge_id
        WHERE c.challenge_id = ?
        ORDER BY cp.part_number
    )", {std::to_string(challenge_id)});

    if (results.empty()) {
        return std::nullopt;
    }

    Challenge challenge;
    challenge.challenge_id = std::stoull(results[0][0]);
    challenge.title = results[0][1];
    challenge.description = results[0][2];

    for (const auto& row : results) {
        if (!row[3].empty()) {
            ChallengePart part;
            part.part_number = std::stoi(row[3]);
            part.description = row[4];
            part.answer = row[5];
            part.points = std::stoi(row[6]);
            challenge.parts.push_back(part);
        }
    }

    return challenge;
}

bool CringeChallengeStore::submitAnswer(dpp::snowflake user_id, dpp::snowflake challenge_id, 
                                      int part, const std::string& answer) {
    // Check if previous parts are completed
    if (part > 1) {
        auto results = db.query(
            "SELECT COUNT(*) FROM user_progress "
            "WHERE user_id = ? AND challenge_id = ? AND part_number = ?",
            {
                std::to_string(user_id),
                std::to_string(challenge_id),
                std::to_string(part - 1)
            }
        );

        if (results.empty() || results[0].empty() || std::stoi(results[0][0]) == 0) {
            return false;
        }
    }

    // Check answer and record progress
    db.beginTransaction();
    try {
        auto results = db.query(
            "SELECT points FROM challenge_parts "
            "WHERE challenge_id = ? AND part_number = ? AND answer = ? "
            "AND NOT EXISTS ("
                "SELECT 1 FROM user_progress "
                "WHERE user_id = ? AND challenge_id = ? AND part_number = ?"
            ")",
            {
                std::to_string(challenge_id),
                std::to_string(part),
                answer,
                std::to_string(user_id),
                std::to_string(challenge_id),
                std::to_string(part)
            }
        );

        if (results.empty() || results[0].empty()) {
            db.rollback();
            return false;
        }

        int points = std::stoi(results[0][0]);

        db.execute(
            "INSERT INTO user_progress (user_id, challenge_id, part_number, points_earned) "
            "VALUES (?, ?, ?, ?)",
            {
                std::to_string(user_id),
                std::to_string(challenge_id),
                std::to_string(part),
                std::to_string(points)
            }
        );

        db.commit();
        return true;
    } catch (...) {
        db.rollback();
        throw;
    }
}

std::vector<std::pair<dpp::snowflake, int>> CringeChallengeStore::getLeaderboard(bool global, dpp::snowflake guild_id) {
    auto results = db.query(
        "SELECT user_id, SUM(points_earned) as total_points "
        "FROM user_progress "
        "GROUP BY user_id "
        "ORDER BY total_points DESC "
        "LIMIT 10"
    );

    std::vector<std::pair<dpp::snowflake, int>> leaderboard;
    for (const auto& row : results) {
        if (!row.empty()) {
            leaderboard.emplace_back(
                std::stoull(row[0]),
                std::stoi(row[1])
            );
        }
    }

    return leaderboard;
}

std::tuple<int, int, int> CringeChallengeStore::getUserStats(dpp::snowflake user_id) {
    auto results = db.query(
        "SELECT "
        "SUM(points_earned) as total_points, "
        "COUNT(DISTINCT challenge_id) as challenges_attempted, "
        "COUNT(DISTINCT CASE WHEN part_number = "
            "(SELECT MAX(part_number) FROM challenge_parts cp "
            "WHERE cp.challenge_id = user_progress.challenge_id) "
        "THEN challenge_id END) as challenges_completed "
        "FROM user_progress "
        "WHERE user_id = ?",
        {std::to_string(user_id)}
    );

    if (results.empty() || results[0].empty()) {
        return {0, 0, 0};
    }

    return {
        std::stoi(results[0][0]), // total_points
        std::stoi(results[0][1]), // challenges_attempted
        std::stoi(results[0][2])  // challenges_completed
    };
}

bool CringeChallengeStore::isPartCompleted(dpp::snowflake user_id, dpp::snowflake challenge_id, int part) {
    auto results = db.query(
        "SELECT 1 FROM user_progress "
        "WHERE user_id = ? AND challenge_id = ? AND part_number = ?",
        {
            std::to_string(user_id),
            std::to_string(challenge_id),
            std::to_string(part)
        }
    );

    return !results.empty();
}

int CringeChallengeStore::getCurrentPart(dpp::snowflake user_id, dpp::snowflake challenge_id) {
    auto results = db.query(
        "SELECT MAX(part_number) FROM user_progress "
        "WHERE user_id = ? AND challenge_id = ?",
        {
            std::to_string(user_id),
            std::to_string(challenge_id)
        }
    );

    if (results.empty() || results[0].empty() || results[0][0].empty()) {
        return 1; // Start with part 1 if no progress
    }

    return std::stoi(results[0][0]) + 1; // Next part after the last completed one
}
