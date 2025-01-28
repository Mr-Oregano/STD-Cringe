#ifndef CRINGE_MESSAGE_STORE_H
#define CRINGE_MESSAGE_STORE_H

#include "utils/database/cringe_database.h"
#include <dpp/dpp.h>
#include <optional>
#include <vector>
#include <string>
#include <chrono>

struct MessageData {
    dpp::snowflake id;
    dpp::snowflake author_id;
    dpp::snowflake guild_id;
    dpp::snowflake channel_id;
    std::string content;
    std::time_t timestamp;
    std::optional<std::time_t> edited_timestamp;
    bool tts;
    bool mention_everyone;
    bool pinned;
    std::optional<dpp::snowflake> webhook_id;
    std::string attachments;  // JSON string
    std::string embeds;       // JSON string
    std::string reactions;    // JSON string
    std::optional<dpp::snowflake> referenced_message_id;
    uint32_t flags;
};

struct GuildMessage {
    dpp::snowflake id;
    dpp::snowflake author_id;
    std::string author_name;
    std::string content;
    std::time_t timestamp;
    dpp::snowflake channel_id;
    bool pinned;
    std::string attachments;  // JSON string
    std::string reactions;    // JSON string
    std::optional<dpp::snowflake> referenced_message_id;
};

struct MessageInfo {
	std::string content;
	std::string author_name;
	dpp::snowflake author_id;
};

class MessageStore {
private:
    CringeDB& db;
    void initTables();
    MessageData rowToMessage(const std::vector<std::string>& row) const;
    dpp::message messageDataToMessage(const MessageData& data) const;

public:
    explicit MessageStore(CringeDB& database);

    // Message operations
    void storeMessage(const dpp::message& message);
    void storeMessages(const std::vector<dpp::message>& messages);
    void deleteMessage(dpp::snowflake message_id);
	size_t getUserMessageCount(uint64_t user_id, uint64_t guild_id);
    
    // User operations
    void storeUser(const dpp::user& user) const;
    void updateUserActivity(dpp::snowflake user_id);
    
    // Retrieval operations
    std::vector<std::string> getUserMessages(
        dpp::snowflake user_id,
        dpp::snowflake guild_id,
        std::optional<size_t> limit = std::nullopt
    );
    
    // New retrieval methods
    std::optional<dpp::message> getLatestUserMessage(
        dpp::snowflake user_id,
        dpp::snowflake guild_id
    );

    std::optional<dpp::message> getOldestUserMessage(
        dpp::snowflake user_id,
        dpp::snowflake guild_id
    );

    std::vector<dpp::message> getUserMessageRange(
        dpp::snowflake user_id,
        dpp::snowflake guild_id,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end,
        std::optional<size_t> limit = std::nullopt
    );

	std::optional<MessageInfo> getMessageById(dpp::snowflake message_id);

    std::vector<std::pair<dpp::snowflake, size_t>> getTopPosters(
        dpp::snowflake guild_id,
        size_t limit = 10
    );

    std::vector<std::string> getMostRecentMessages(
        dpp::snowflake guild_id,
        dpp::snowflake user_id,
        size_t limit
    );

    std::vector<std::string> getMostRecentMessagesWithContent(
        dpp::snowflake guild_id,
        dpp::snowflake user_id,
        size_t limit
    );

    std::vector<GuildMessage> getRecentGuildMessages(
        dpp::snowflake guild_id,
        size_t limit
    );
    
    // History operations
    void fetchAndStoreHistory(
        dpp::cluster& cluster,
        dpp::snowflake channel_id,
        dpp::snowflake guild_id,
        size_t& total_messages
    );

    std::vector<dpp::snowflake> getActiveChannels(dpp::snowflake guild_id, size_t limit = 10);
    void clearGuildMessages(dpp::snowflake guild_id);
    std::vector<std::string> searchMessages(
        dpp::snowflake guild_id,
        const std::string& query,
        std::optional<size_t> limit = std::nullopt
    );
    
    std::unordered_map<std::string, size_t> getMessageStats(
        dpp::snowflake guild_id,
        dpp::snowflake user_id,
        std::chrono::system_clock::time_point since = std::chrono::system_clock::now() - 
            std::chrono::hours(24 * 30)  // Default to last 30 days
    );

    void syncGuildMessages(
        dpp::cluster& cluster,
        dpp::snowflake guild_id,
        const std::function<void(const std::string&, size_t, size_t)>& progress_callback
    );
};

#endif