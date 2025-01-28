#include "utils/database/conspiracy_store.h"
#include <fmt/format.h>
#include <chrono>

ConspiracyStore::ConspiracyStore(CringeDB& database) : db(database) {
    initTables();
}

void ConspiracyStore::initTables() {
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS conspiracies (
            id INTEGER PRIMARY KEY,
            guild_id INTEGER NOT NULL,
            creator_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            involved_users TEXT NOT NULL,
            evidence_messages TEXT NOT NULL,
            complexity_level INTEGER DEFAULT 1,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL
        )
    )");

    // Create indices
    db.createIndex("idx_conspiracies_guild", "conspiracies", "guild_id");
    db.createIndex("idx_conspiracies_creator", "conspiracies", "creator_id");
    db.createIndex("idx_conspiracies_updated", "conspiracies", "updated_at");
}

dpp::snowflake ConspiracyStore::createConspiracy(
    dpp::snowflake guild_id,
    dpp::snowflake creator_id,
    const std::string& title,
    const std::string& content,
    const std::vector<dpp::snowflake>& involved_users,
    const std::vector<dpp::snowflake>& evidence_messages
) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);

    // Convert vectors to JSON strings
    std::string users_json = "[" + fmt::format("{}", fmt::join(involved_users, ",")) + "]";
    std::string messages_json = "[" + fmt::format("{}", fmt::join(evidence_messages, ",")) + "]";

    // Insert conspiracy
    db.execute(
        "INSERT INTO conspiracies "
        "(guild_id, creator_id, title, content, involved_users, evidence_messages, "
        "complexity_level, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, 1, ?, ?)",
        {
            std::to_string(guild_id),
            std::to_string(creator_id),
            title,
            content,
            users_json,
            messages_json,
            std::to_string(timestamp),
            std::to_string(timestamp)
        }
    );

    // Get the ID of the newly inserted conspiracy
    auto results = db.query("SELECT last_insert_rowid()");
    return std::stoull(results[0][0]);
}

std::optional<ConspiracyTheory> ConspiracyStore::getConspiracy(dpp::snowflake id) {
	auto results = db.query(
		"SELECT * FROM conspiracies WHERE id = ?",
		{std::to_string(id)}
	);

	if (results.empty()) {
		return std::nullopt;
	}

	ConspiracyTheory theory;
	const auto& row = results[0];

	theory.id = std::stoull(row[0]);
	theory.guild_id = std::stoull(row[1]);
	theory.creator_id = std::stoull(row[2]);
	theory.title = row[3];
	theory.content = row[4];
	theory.complexity_level = std::stoi(row[7]);

	// Convert Unix timestamps to time_points
	auto created_timestamp = std::stoull(row[8]);
	auto updated_timestamp = std::stoull(row[9]);
	theory.created_at = std::chrono::system_clock::from_time_t(created_timestamp);
	theory.updated_at = std::chrono::system_clock::from_time_t(updated_timestamp);

	// Parse involved users JSON
	std::string users_str = row[5];
	if (users_str.length() > 2) {
		users_str = users_str.substr(1, users_str.length() - 2);
		std::istringstream users_stream(users_str);
		std::string user_id;
		while (std::getline(users_stream, user_id, ',')) {
			if (!user_id.empty()) {
				theory.involved_users.push_back(std::stoull(user_id));
			}
		}
	}

	// Parse evidence messages JSON
	std::string messages_str = row[6];
	if (messages_str.length() > 2) {
		messages_str = messages_str.substr(1, messages_str.length() - 2);
		std::istringstream messages_stream(messages_str);
		std::string message_id;
		while (std::getline(messages_stream, message_id, ',')) {
			if (!message_id.empty()) {
				theory.evidence_messages.push_back(std::stoull(message_id));
			}
		}
	}

	return theory;
}

std::vector<ConspiracyTheory> ConspiracyStore::getGuildConspiracies(dpp::snowflake guild_id) {
    auto results = db.query(
        "SELECT * FROM conspiracies WHERE guild_id = ? ORDER BY updated_at DESC",
        {std::to_string(guild_id)}
    );

    std::vector<ConspiracyTheory> theories;
    theories.reserve(results.size());

    for (const auto& row : results) {
        auto theory = getConspiracy(std::stoull(row[0]));
        if (theory) {
            theories.push_back(*theory);
        }
    }

    return theories;
}

bool ConspiracyStore::updateConspiracy(const ConspiracyTheory& theory) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);

    std::string users_json = "[" + fmt::format("{}", fmt::join(theory.involved_users, ",")) + "]";
    std::string messages_json = "[" + fmt::format("{}", fmt::join(theory.evidence_messages, ",")) + "]";

    try {
        db.execute(
            "UPDATE conspiracies SET "
            "title = ?, content = ?, involved_users = ?, evidence_messages = ?, "
            "complexity_level = ?, updated_at = ? "
            "WHERE id = ?",
            {
                theory.title,
                theory.content,
                users_json,
                messages_json,
                std::to_string(theory.complexity_level),
                std::to_string(timestamp),
                std::to_string(theory.id)
            }
        );
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool ConspiracyStore::deleteConspiracy(dpp::snowflake id) {
    try {
        db.execute("DELETE FROM conspiracies WHERE id = ?", {std::to_string(id)});
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool ConspiracyStore::incrementComplexity(dpp::snowflake id) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);

    try {
        db.execute(
            "UPDATE conspiracies SET "
            "complexity_level = complexity_level + 1, "
            "updated_at = ? "
            "WHERE id = ?",
            {
                std::to_string(timestamp),
                std::to_string(id)
            }
        );
        return true;
    } catch (const std::exception&) {
        return false;
    }
}