#ifndef CRINGE_CHAT_STORE_H
#define CRINGE_CHAT_STORE_H

#include "utils/database/cringe_database.h"
#include <dpp/dpp.h>
#include <vector>
#include <string>

struct ChatMessage {
    dpp::snowflake id;
    dpp::snowflake conversation_id;
    dpp::snowflake user_id;
    std::string content;
    std::string response;
    std::chrono::system_clock::time_point timestamp;
    std::string model;
};

struct ChatSummary {
    uint64_t id;
    uint64_t user_id;
    std::string model;
    std::string title;
    std::string last_message;
    std::chrono::system_clock::time_point last_updated;
    size_t message_count;
};

class ChatStore {
private:
    CringeDB& db;
    void initTables();

public:
    explicit ChatStore(CringeDB& database);
    
    // Conversation operations
    dpp::snowflake createConversation(dpp::snowflake user_id, dpp::snowflake guild_id, const std::string& model);
    void storeMessage(const ChatMessage& message);
    std::vector<ChatMessage> getConversationHistory(dpp::snowflake conversation_id, size_t limit = 10);
    bool isValidConversation(dpp::snowflake conversation_id, dpp::snowflake user_id);
    std::vector<ChatSummary> getUserChats(dpp::snowflake user_id);
    std::vector<ChatSummary> getServerChats(dpp::snowflake guild_id);
    void updateConversationTitle(uint64_t conversation_id, const std::string& title);
    std::string getConversationTitle(uint64_t conversation_id);
    void updateMessage(const ChatMessage& message);
    void deleteConversation(dpp::snowflake conversation_id);
};

#endif