#include "utils/database/chat_store.h"

ChatStore::ChatStore(CringeDB& database) : db(database) {
    initTables();
}

void ChatStore::initTables() {
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS conversations (
            id INTEGER PRIMARY KEY,
            user_id INTEGER NOT NULL,
            guild_id INTEGER NOT NULL,
            model TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            title TEXT NOT NULL
        )
    )");

    db.execute(R"(
        CREATE TABLE IF NOT EXISTS chat_messages (
            id INTEGER PRIMARY KEY,
            conversation_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            response TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            model TEXT NOT NULL,
            FOREIGN KEY(conversation_id) REFERENCES conversations(id)
        )
    )");
}

dpp::snowflake ChatStore::createConversation(dpp::snowflake user_id, dpp::snowflake guild_id, const std::string& model) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    db.execute(
        "INSERT INTO conversations (user_id, guild_id, model, created_at, title) VALUES (?, ?, ?, ?, ?)",
        {
            std::to_string(user_id), 
            std::to_string(guild_id), 
            model, 
            std::to_string(timestamp),
            "Chat with " + model
        }
    );
    
    auto result = db.query("SELECT last_insert_rowid()");
    return std::stoull(result[0][0]);
}

void ChatStore::storeMessage(const ChatMessage& message) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    db.execute(
        "INSERT INTO chat_messages (conversation_id, user_id, content, response, timestamp, model) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {
            std::to_string(message.conversation_id),
            std::to_string(message.user_id),
            message.content,
            message.response,
            std::to_string(timestamp),
            message.model
        }
    );
}

std::vector<ChatMessage> ChatStore::getConversationHistory(dpp::snowflake conversation_id, size_t limit) {
    return db.executeWithTimeout([&]() {
        auto results = db.query(
            "SELECT id, conversation_id, user_id, content, response, timestamp, model "
            "FROM chat_messages "
            "WHERE conversation_id = ? "
            "ORDER BY timestamp ASC LIMIT ?",
            {std::to_string(conversation_id), std::to_string(limit)}
        );

        std::vector<ChatMessage> messages;
        for (const auto& row : results) {
            ChatMessage msg{
                std::stoull(row[0]),
                std::stoull(row[1]),
                std::stoull(row[2]),
                row[3],
                row[4],
                std::chrono::system_clock::from_time_t(std::stoull(row[5])),
                row[6]
            };
            messages.push_back(msg);
        }
        return messages;
    }, std::chrono::milliseconds(5000)); // 5 second timeout
}

bool ChatStore::isValidConversation(dpp::snowflake conversation_id, dpp::snowflake user_id) {
    auto results = db.query(
        "SELECT 1 FROM conversations WHERE id = ? AND user_id = ?",
        {std::to_string(conversation_id), std::to_string(user_id)}
    );
    return !results.empty();
}

std::vector<ChatSummary> ChatStore::getUserChats(dpp::snowflake user_id) {
    std::vector<ChatSummary> chats;
    
    auto results = db.query(
        R"(
            SELECT 
                c.id,
                c.user_id,
                c.model,
                c.title,
                m.content as last_message,
                m.timestamp as last_updated,
                COUNT(m2.id) as message_count
            FROM conversations c
            LEFT JOIN chat_messages m ON m.conversation_id = c.id 
                AND m.id = (
                    SELECT id FROM chat_messages 
                    WHERE conversation_id = c.id 
                    ORDER BY timestamp DESC LIMIT 1
                )
            LEFT JOIN chat_messages m2 ON m2.conversation_id = c.id
            WHERE c.user_id = ?
            GROUP BY c.id, c.title
            ORDER BY m.timestamp DESC
        )",
        {std::to_string(user_id)}
    );
    
    for (const auto& row : results) {
        ChatSummary summary{
            std::stoull(row[0]),  // id
            std::stoull(row[1]),  // user_id
            row[2],               // model
            row[3],               // title
            row[4],               // last_message
            std::chrono::system_clock::from_time_t(std::stoull(row[5])), // last_updated
            std::stoull(row[6])   // message_count
        };
        chats.push_back(summary);
    }
    
    return chats;
}

std::vector<ChatSummary> ChatStore::getServerChats(dpp::snowflake guild_id) {
    std::vector<ChatSummary> chats;
    
    auto results = db.query(
        R"(
            SELECT 
                c.id,
                c.user_id,
                c.model,
                c.title,
                m.content as last_message,
                m.timestamp as last_updated,
                COUNT(m2.id) as message_count
            FROM conversations c
            LEFT JOIN chat_messages m ON m.conversation_id = c.id 
                AND m.id = (
                    SELECT id FROM chat_messages 
                    WHERE conversation_id = c.id 
                    ORDER BY timestamp DESC LIMIT 1
                )
            LEFT JOIN chat_messages m2 ON m2.conversation_id = c.id
            WHERE c.guild_id = ?
            GROUP BY c.id, c.title
            ORDER BY m.timestamp DESC
        )",
        {std::to_string(guild_id)}
    );
    
    for (const auto& row : results) {
        ChatSummary summary{
            std::stoull(row[0]),  // id
            std::stoull(row[1]),  // user_id
            row[2],               // model
            row[3],               // title
            row[4],               // last_message
            std::chrono::system_clock::from_time_t(std::stoull(row[5])), // last_updated
            std::stoull(row[6])   // message_count
        };
        chats.push_back(summary);
    }
    
    return chats;
}

void ChatStore::updateConversationTitle(uint64_t conversation_id, const std::string& title) {
	db.execute(
		"UPDATE conversations SET title = ? WHERE id = ?",
		{title, std::to_string(conversation_id)}
	);
}

std::string ChatStore::getConversationTitle(uint64_t conversation_id) {
	auto result = db.query(
		"SELECT title FROM conversations WHERE id = ?",
		{std::to_string(conversation_id)}
	);

	if (!result.empty()) {
		return result[0][0];
	}
	return "";
}

void ChatStore::updateMessage(const ChatMessage& message) {
    db.execute(
        "UPDATE chat_messages SET response = ? WHERE id = ?",
        {message.response, std::to_string(message.id)}
    );
}

void ChatStore::deleteConversation(dpp::snowflake conversation_id) {
    // Delete all messages first due to foreign key constraint
    db.execute(
        "DELETE FROM chat_messages WHERE conversation_id = ?",
        {std::to_string(conversation_id)}
    );
    
    // Then delete the conversation
    db.execute(
        "DELETE FROM conversations WHERE id = ?",
        {std::to_string(conversation_id)}
    );
}