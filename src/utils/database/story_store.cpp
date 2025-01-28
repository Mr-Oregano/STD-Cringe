#include "utils/database/story_store.h"
#include "utils/database/story_store.h"
#include <fmt/format.h>

StoryStore::StoryStore(CringeDB& database) : db(database) {
    initTables();
}

void StoryStore::initTables() {
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS stories (
            story_id INTEGER PRIMARY KEY,
            guild_id INTEGER NOT NULL,
            channel_id INTEGER NOT NULL,
            theme TEXT NOT NULL,
            title TEXT NOT NULL,
            max_participants INTEGER NOT NULL,
            current_turn INTEGER DEFAULT 0,
            last_update INTEGER NOT NULL,
            is_active INTEGER DEFAULT 1
        )
    )");

    db.execute(R"(
        CREATE TABLE IF NOT EXISTS story_participants (
            story_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            join_order INTEGER NOT NULL,
            PRIMARY KEY (story_id, user_id),
            FOREIGN KEY (story_id) REFERENCES stories(story_id)
        )
    )");

    db.execute(R"(
        CREATE TABLE IF NOT EXISTS story_parts (
            story_id INTEGER NOT NULL,
            part_number INTEGER NOT NULL,
            content TEXT NOT NULL,
            author_id INTEGER NOT NULL,
            created_at INTEGER NOT NULL,
            PRIMARY KEY (story_id, part_number),
            FOREIGN KEY (story_id) REFERENCES stories(story_id)
        )
    )");

    // Create indices
    db.createIndex("idx_stories_channel", "stories", "channel_id");
    db.createIndex("idx_stories_active", "stories", "is_active");
    db.createIndex("idx_stories_last_update", "stories", "last_update");
}

dpp::snowflake StoryStore::createStory(
    dpp::snowflake guild_id,
    dpp::snowflake channel_id,
    const std::string& theme,
    const std::string& title,
    dpp::snowflake creator_id,
    size_t max_participants,
    const std::string& initial_part
) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);

    // Insert story
    db.execute(
        "INSERT INTO stories (guild_id, channel_id, theme, title, max_participants, last_update) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {
            std::to_string(guild_id),
            std::to_string(channel_id),
            theme,
            title,
            std::to_string(max_participants),
            std::to_string(timestamp)
        }
    );

    // Get the ID of the newly inserted story using a query
    auto results = db.query("SELECT last_insert_rowid()");
    dpp::snowflake story_id = std::stoull(results[0][0]);

    // Add creator as first participant
    db.execute(
        "INSERT INTO story_participants (story_id, user_id, join_order) VALUES (?, ?, 0)",
        {std::to_string(story_id), std::to_string(creator_id)}
    );

    // Add initial story part
    db.execute(
        "INSERT INTO story_parts (story_id, part_number, content, author_id, created_at) "
        "VALUES (?, 0, ?, ?, ?)",
        {
            std::to_string(story_id),
            initial_part,
            std::to_string(creator_id),
            std::to_string(timestamp)
        }
    );

    return story_id;
}

std::optional<Story> StoryStore::getStory(dpp::snowflake story_id) {
    // Get story details
    auto story_results = db.query(
        "SELECT story_id, guild_id, channel_id, theme, title, max_participants, "
        "current_turn, last_update, is_active FROM stories WHERE story_id = ?",
        {std::to_string(story_id)}
    );

    if (story_results.empty()) {
        return std::nullopt;
    }

    // Get participants
    auto participant_results = db.query(
        "SELECT user_id FROM story_participants WHERE story_id = ? ORDER BY join_order",
        {std::to_string(story_id)}
    );

    // Get story parts
    auto parts_results = db.query(
        "SELECT content FROM story_parts WHERE story_id = ? ORDER BY part_number",
        {std::to_string(story_id)}
    );

    Story story;
    story.story_id = std::stoull(story_results[0][0]);
    story.guild_id = std::stoull(story_results[0][1]);
    story.channel_id = std::stoull(story_results[0][2]);
    story.theme = story_results[0][3];
    story.title = story_results[0][4];
    story.max_participants = std::stoull(story_results[0][5]);
    story.current_turn = std::stoull(story_results[0][6]);
    story.last_update = std::chrono::system_clock::from_time_t(std::stoull(story_results[0][7]));
    story.is_active = std::stoi(story_results[0][8]) != 0;

    // Add participants
    for (const auto& row : participant_results) {
        story.participants.push_back(std::stoull(row[0]));
    }

    // Add story parts
    for (const auto& row : parts_results) {
        story.story_parts.push_back(row[0]);
    }

    return story;
}

std::vector<Story> StoryStore::getActiveStoriesInChannel(dpp::snowflake channel_id) {
    std::vector<Story> stories;

    auto story_ids = db.query(
        "SELECT story_id FROM stories WHERE channel_id = ? AND is_active = 1",
        {std::to_string(channel_id)}
    );

    for (const auto& row : story_ids) {
        auto story = getStory(std::stoull(row[0]));
        if (story) {
            stories.push_back(*story);
        }
    }

    return stories;
}

bool StoryStore::addParticipant(dpp::snowflake story_id, dpp::snowflake user_id) {
    try {
        // Check if story exists and is active
        auto story = getStory(story_id);
        if (!story || !story->is_active) {
            return false;
        }

        // Check if max participants reached
        if (story->participants.size() >= story->max_participants) {
            return false;
        }

        // Get next join order
        auto join_order = story->participants.size();

        // Add participant
        db.execute(
            "INSERT INTO story_participants (story_id, user_id, join_order) VALUES (?, ?, ?)",
            {
                std::to_string(story_id),
                std::to_string(user_id),
                std::to_string(join_order)
            }
        );

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool StoryStore::addStoryPart(dpp::snowflake story_id, dpp::snowflake user_id, const std::string& content) {
    try {
        auto story = getStory(story_id);
        if (!story || !story->is_active) {
            return false;
        }

        // Verify it's the user's turn
        size_t expected_author_index = story->current_turn % story->participants.size();
        if (story->participants[expected_author_index] != user_id) {
            return false;
        }

        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);

        // Add story part
        db.execute(
            "INSERT INTO story_parts (story_id, part_number, content, author_id, created_at) "
            "VALUES (?, ?, ?, ?, ?)",
            {
                std::to_string(story_id),
                std::to_string(story->story_parts.size()),
                content,
                std::to_string(user_id),
                std::to_string(timestamp)
            }
        );

        // Update current turn and last_update
        db.execute(
            "UPDATE stories SET current_turn = current_turn + 1, last_update = ? "
            "WHERE story_id = ?",
            {
                std::to_string(timestamp),
                std::to_string(story_id)
            }
        );

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool StoryStore::updateTitle(dpp::snowflake story_id, const std::string& new_title) {
    try {
        auto story = getStory(story_id);
        if (!story || !story->is_active) {
            return false;
        }

        db.execute(
            "UPDATE stories SET title = ? WHERE story_id = ?",
            {new_title, std::to_string(story_id)}
        );

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool StoryStore::markInactive(dpp::snowflake story_id) {
    try {
        db.execute(
            "UPDATE stories SET is_active = 0 WHERE story_id = ?",
            {std::to_string(story_id)}
        );
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void StoryStore::cleanupOldStories(std::chrono::hours age) {
    auto now = std::chrono::system_clock::now();
    auto cutoff = std::chrono::system_clock::to_time_t(now - age);

    db.execute(
        "UPDATE stories SET is_active = 0 WHERE last_update < ? AND is_active = 1",
        {std::to_string(cutoff)}
    );
}
