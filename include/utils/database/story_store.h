#ifndef CRINGE_STORY_STORE_H
#define CRINGE_STORY_STORE_H

#include "utils/database/cringe_database.h"
#include <dpp/dpp.h>
#include <vector>
#include <string>
#include <optional>
#include <chrono>

struct Story {
	dpp::snowflake story_id;
	dpp::snowflake guild_id;
	dpp::snowflake channel_id;
	std::string theme;
	std::string title;
	std::vector<dpp::snowflake> participants;
	std::vector<std::string> story_parts;
	size_t current_turn;
	size_t max_participants;
	std::chrono::system_clock::time_point last_update;
	bool is_active;
};

class StoryStore {
private:
	CringeDB& db;
	void initTables();

public:
	explicit StoryStore(CringeDB& database);

	// Create a new story
	dpp::snowflake createStory(
		dpp::snowflake guild_id,
		dpp::snowflake channel_id,
		const std::string& theme,
		const std::string& title,
		dpp::snowflake creator_id,
		size_t max_participants,
		const std::string& initial_part
	);

	// Get story by ID
	std::optional<Story> getStory(dpp::snowflake story_id);

	// Get active stories in a channel
	std::vector<Story> getActiveStoriesInChannel(dpp::snowflake channel_id);

	// Add participant to story
	bool addParticipant(dpp::snowflake story_id, dpp::snowflake user_id);

	// Add story part
	bool addStoryPart(dpp::snowflake story_id, dpp::snowflake user_id, const std::string& content);

	// Update story title
	bool updateTitle(dpp::snowflake story_id, const std::string& new_title);

	// Mark story as inactive
	bool markInactive(dpp::snowflake story_id);

	// Clean up old inactive stories
	void cleanupOldStories(std::chrono::hours age = std::chrono::hours(24 * 7));
};

#endif