#include "utils/database/rage_store.h"
#include <fmt/format.h>

RageStore::RageStore(CringeDB& database) : db(database) {
    initTables();
}

void RageStore::initTables() {
    // Create rage_interactions table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS rage_interactions (
            message_id INTEGER PRIMARY KEY,
            user_id INTEGER NOT NULL,
            guild_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            reaction TEXT NOT NULL,
            anger_score REAL NOT NULL,
            timestamp INTEGER NOT NULL,
            referenced_message_id INTEGER,
            is_bot_message BOOLEAN DEFAULT 0,
            FOREIGN KEY(referenced_message_id) REFERENCES rage_interactions(message_id)
        )
    )");

    // Create rage_bait table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS rage_bait (
            message_id INTEGER PRIMARY KEY,
            channel_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            created_at INTEGER NOT NULL
        )
    )");

    // Create indices
    db.createIndex("idx_rage_user", "rage_interactions", "user_id");
    db.createIndex("idx_rage_guild", "rage_interactions", "guild_id");
    db.createIndex("idx_rage_timestamp", "rage_interactions", "timestamp");
}

void RageStore::storeRageInteraction(const RageInteraction& interaction) {
    // First check if the referenced message exists (if there is one)
    if (interaction.referenced_message_id != 0) {
        auto ref_check = db.query(
            "SELECT 1 FROM rage_interactions WHERE message_id = ?",
            {std::to_string(interaction.referenced_message_id)}
        );
        
        if (ref_check.empty()) {
            std::cout << "Warning: Referenced message " << interaction.referenced_message_id 
                        << " does not exist in database" << std::endl;
            return;
        }
    }
    
	auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
		interaction.timestamp.time_since_epoch()
	).count();

	db.execute(
        "INSERT INTO rage_interactions "
        "(message_id, user_id, guild_id, content, reaction, anger_score, timestamp, referenced_message_id, is_bot_message) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            std::to_string(interaction.message_id),
            std::to_string(interaction.user_id),
            std::to_string(interaction.guild_id),
            interaction.content,
            interaction.reaction,
            std::to_string(interaction.anger_score),
            std::to_string(timestamp),
            std::to_string(interaction.referenced_message_id),
            interaction.is_bot_message ? "1" : "0"
        }
    );
}

void RageStore::storeRageBait(dpp::snowflake message_id, dpp::snowflake channel_id, const std::string& content) {
	auto now = std::chrono::system_clock::now();
	auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
		now.time_since_epoch()
	).count();

	db.execute(
		"INSERT INTO rage_bait (message_id, channel_id, content, created_at) "
		"VALUES (?, ?, ?, ?)",
		std::vector<std::string>{
			std::to_string(message_id),
			std::to_string(channel_id),
			content,
			std::to_string(timestamp)
		}
	);
}

std::vector<RageInteraction> RageStore::getRecentInteractions(dpp::snowflake user_id, size_t limit) {
    auto results = db.query(
        "SELECT message_id, user_id, guild_id, content, reaction, anger_score, timestamp "
        "FROM rage_interactions "
        "WHERE user_id = ? "
        "ORDER BY timestamp DESC LIMIT ?",
        {std::to_string(user_id), std::to_string(limit)}
    );

    std::vector<RageInteraction> interactions;
    for (const auto& row : results) {
        RageInteraction interaction{
            std::stoull(row[0]), // message_id
            std::stoull(row[1]), // user_id
            std::stoull(row[2]), // guild_id
            row[3],              // content
            row[4],              // reaction
            std::stod(row[5]),   // anger_score
            std::chrono::system_clock::from_time_t(std::stoull(row[6])) // timestamp
        };
        interactions.push_back(interaction);
    }
    return interactions;
}

std::vector<RageInteraction> RageStore::getGuildInteractions(dpp::snowflake guild_id, size_t limit) {
    auto results = db.query(
        "SELECT message_id, user_id, guild_id, content, reaction, anger_score, timestamp "
        "FROM rage_interactions "
        "WHERE guild_id = ? "
        "ORDER BY timestamp DESC LIMIT ?",
        {std::to_string(guild_id), std::to_string(limit)}
    );

    std::vector<RageInteraction> interactions;
    for (const auto& row : results) {
        RageInteraction interaction{
            std::stoull(row[0]), // message_id
            std::stoull(row[1]), // user_id
            std::stoull(row[2]), // guild_id
            row[3],              // content
            row[4],              // reaction
            std::stod(row[5]),   // anger_score
            std::chrono::system_clock::from_time_t(std::stoull(row[6])) // timestamp
        };
        interactions.push_back(interaction);
    }
    return interactions;
}

double RageStore::getAngerScore(dpp::snowflake user_id) {
    auto result = db.query(
        "SELECT AVG(anger_score) as avg_score FROM rage_interactions WHERE user_id = ?",
        {std::to_string(user_id)}
    );
    return result.empty() || result[0].empty() ? 0.0 : std::stod(result[0][0]);
}

double RageStore::getGuildAngerScore(dpp::snowflake guild_id) {
    auto result = db.query(
        "SELECT AVG(anger_score) as avg_score FROM rage_interactions WHERE guild_id = ?",
        {std::to_string(guild_id)}
    );
    return result.empty() || result[0].empty() ? 0.0 : std::stod(result[0][0]);
}

void RageStore::cleanupOldInteractions(std::chrono::hours age) {
    auto now = std::chrono::system_clock::now();
    auto cutoff = std::chrono::duration_cast<std::chrono::seconds>(
        (now - age).time_since_epoch()
    ).count();

    db.execute(
        "DELETE FROM rage_interactions WHERE timestamp < ?",
        {std::to_string(cutoff)}
    );

    db.execute(
        "DELETE FROM rage_bait WHERE created_at < ?",
        {std::to_string(cutoff)}
    );
}

std::vector<RageInteraction> RageStore::getConversationThread(
    dpp::snowflake message_id,
    dpp::snowflake user_id
) {
    std::cout << "Getting conversation thread for user " << user_id 
              << " starting from message: " << message_id << std::endl;
    std::vector<RageInteraction> thread;

    // First, find the initial bait message by following references up
    auto current_id = message_id;
    dpp::snowflake initial_bait_id = 0;

    // Step 1: Find the initial bait message
    while (current_id != 0) {
        auto results = db.query(
            "SELECT message_id, user_id, guild_id, content, reaction, "
            "anger_score, timestamp, referenced_message_id, is_bot_message "
            "FROM rage_interactions "
            "WHERE message_id = ?",
            {std::to_string(current_id)}
        );

        if (results.empty()) break;

        RageInteraction interaction{
            std::stoull(results[0][0]),  // message_id
            std::stoull(results[0][1]),  // user_id
            std::stoull(results[0][2]),  // guild_id
            results[0][3],               // content
            results[0][4],               // reaction
            std::stod(results[0][5]),    // anger_score
            std::chrono::system_clock::from_time_t(std::stoull(results[0][6])),  // timestamp
            results[0][7].empty() ? 0 : std::stoull(results[0][7]),  // referenced_message_id
            results[0][8] == "1"         // is_bot_message
        };

        if (interaction.reaction == "initial_bait") {
            initial_bait_id = interaction.message_id;
            thread.push_back(interaction);
            break;
        }
        current_id = interaction.referenced_message_id;
    }

    if (initial_bait_id == 0) {
        std::cout << "No initial bait found in chain" << std::endl;
        return thread;
    }

    // Step 2: Get all messages in the conversation between this user and the bot
    auto results = db.query(
        "SELECT message_id, user_id, guild_id, content, reaction, "
        "anger_score, timestamp, referenced_message_id, is_bot_message "
        "FROM rage_interactions "
        "WHERE (user_id = ? OR is_bot_message = 1) "
        "AND message_id IN ("
        "    WITH RECURSIVE msg_refs(id) AS ("
        "        SELECT message_id FROM rage_interactions WHERE message_id = ? "  // initial bait
        "        UNION ALL "
        "        SELECT r.message_id "
        "        FROM rage_interactions r, msg_refs m "
        "        WHERE r.referenced_message_id = m.id"
        "    ) SELECT id FROM msg_refs"
        ") ORDER BY timestamp ASC",
        {std::to_string(user_id), std::to_string(initial_bait_id)}
    );

    for (const auto& row : results) {
        RageInteraction interaction{
            std::stoull(row[0]),  // message_id
            std::stoull(row[1]),  // user_id
            std::stoull(row[2]),  // guild_id
            row[3],               // content
            row[4],               // reaction
            std::stod(row[5]),    // anger_score
            std::chrono::system_clock::from_time_t(std::stoull(row[6])),  // timestamp
            row[7].empty() ? 0 : std::stoull(row[7]),  // referenced_message_id
            row[8] == "1"         // is_bot_message
        };

        // Only add if it's not already in the thread (avoid duplicating initial bait)
        if (std::find_if(thread.begin(), thread.end(),
            [&](const RageInteraction& i) { return i.message_id == interaction.message_id; }
        ) == thread.end()) {
            thread.push_back(interaction);
        }

        std::cout << "Found message in chain: " 
                  << "\n  ID: " << interaction.message_id
                  << "\n  User: " << interaction.user_id
                  << "\n  Type: " << interaction.reaction
                  << "\n  References: " << interaction.referenced_message_id
                  << "\n  Is Bot: " << interaction.is_bot_message 
                  << std::endl;
    }

    return thread;
}


dpp::snowflake RageStore::getOriginalBaitId(dpp::snowflake message_id) {
    auto thread = getConversationThread(message_id, 0);  // Pass 0 as user_id to get all messages
    
    // Find the initial bait in the thread
    for (const auto& msg : thread) {
        if (msg.reaction == "initial_bait") {
            std::cout << "Found original bait: " << msg.message_id << std::endl;
            return msg.message_id;
        }
    }

    std::cout << "No original bait found in chain of " << thread.size() << " messages" << std::endl;
    return 0;
}

bool RageStore::isRageBait(dpp::snowflake message_id) {
    auto results = db.query(
        "SELECT 1 FROM rage_interactions "
        "WHERE message_id = ? AND is_bot_message = 1 AND referenced_message_id IS NULL",
        {std::to_string(message_id)}
    );
    return !results.empty();
}