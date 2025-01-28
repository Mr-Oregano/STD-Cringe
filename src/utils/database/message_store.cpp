#include "utils/database/message_store.h"
#include <chrono>
#include <fmt/format.h>

using json = nlohmann::json;

MessageStore::MessageStore(CringeDB &database) : db(database) {
    initTables();
}

void MessageStore::initTables() {
    // Create messages table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY,
            author_id INTEGER NOT NULL,
            author_name TEXT NOT NULL,
            guild_id INTEGER NOT NULL,
            channel_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            edited_timestamp INTEGER,
            tts BOOLEAN DEFAULT 0,
            mention_everyone BOOLEAN DEFAULT 0,
            pinned BOOLEAN DEFAULT 0,
            webhook_id INTEGER,
            attachments TEXT,
            embeds TEXT,
            reactions TEXT,
            referenced_message_id INTEGER,
            FOREIGN KEY(author_id) REFERENCES users(id)
        )
    )");

    // Create users table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY,
            username TEXT NOT NULL,
            discriminator TEXT,
            avatar TEXT,
            last_seen INTEGER NOT NULL
        )
    )");

    // Create indices
    db.createIndex("idx_messages_author", "messages", "author_id");
    db.createIndex("idx_messages_guild", "messages", "guild_id");
    db.createIndex("idx_messages_channel", "messages", "channel_id");
    db.createIndex("idx_messages_timestamp", "messages", "timestamp");
}

void MessageStore::storeMessage(const dpp::message &message) {
    // First store/update the user
    storeUser(message.author);

    // Convert attachments to JSON
    json attachments_json = json::array();
    for (const auto& attachment : message.attachments) {
        attachments_json.push_back({
            {"id", attachment.id},
            {"url", attachment.url},
            {"filename", attachment.filename}
        });
    }

    // Convert embeds to JSON
    json embeds_json = json::array();
    // for (const auto& embed : message.embeds) {
    //     embeds_json.push_back(
    //     	{"author", embed.author},
    //     	{"title", embed.title}
    //     );
    // }

    // Convert reactions to JSON
    json reactions_json = json::array();
    for (const auto& reaction : message.reactions) {
        reactions_json.push_back({
            {"count", reaction.count},
            {"emoji_name", reaction.emoji_name},
            {"emoji_id", reaction.emoji_id}
        });
    }

    // Store the message
   db.execute(
        "INSERT OR REPLACE INTO messages "
        "(id, author_id, author_name, guild_id, channel_id, content, timestamp, "
        "edited_timestamp, tts, mention_everyone, pinned, webhook_id, "
        "attachments, embeds, reactions, referenced_message_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            std::to_string(message.id),
            std::to_string(message.author.id),
            message.author.username,
            std::to_string(message.guild_id),
            std::to_string(message.channel_id),
            message.content,
            std::to_string(message.sent),
            message.edited ? std::to_string(message.edited) : "",
            message.tts ? "1" : "0",
            message.mention_everyone ? "1" : "0",
            message.pinned ? "1" : "0",
            message.webhook_id ? std::to_string(message.webhook_id) : "",
            attachments_json.dump(),
            embeds_json.dump(),
            reactions_json.dump(),
            message.message_reference.message_id ? 
                std::to_string(message.message_reference.message_id) : ""
        }
    );
}

void MessageStore::storeMessages(const std::vector<dpp::message> &messages) {
    db.beginTransaction();
    try {
        for (const auto &message: messages) {
            storeMessage(message);
        }
        db.commit();
    } catch (...) {
        db.rollback();
        throw;
    }
}

void MessageStore::storeUser(const dpp::user &user) const {
    db.execute(
        "INSERT OR REPLACE INTO users "
        "(id, username, discriminator, avatar, last_seen) "
        "VALUES (?, ?, ?, ?, ?)",
        std::vector<std::string>{
            std::to_string(user.id),
            user.username,
            std::to_string(user.discriminator),
            user.avatar.to_string(), // Convert iconhash to string
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count())
        }
    );
}

std::vector<std::string> MessageStore::getUserMessages(
    dpp::snowflake user_id,
    dpp::snowflake guild_id,
    std::optional<size_t> limit
) {
    std::string sql = fmt::format(
        "SELECT content FROM messages "
        "WHERE author_id = ? AND guild_id = ? "
        "ORDER BY timestamp DESC{}",
        limit ? fmt::format(" LIMIT {}", *limit) : ""
    );

    auto results = db.query(sql, {
                                std::to_string(user_id),
                                std::to_string(guild_id)
                            });

    std::vector<std::string> messages;
    messages.reserve(results.size());
    for (const auto &row: results) {
        if (!row.empty()) {
            messages.push_back(row[0]);
        }
    }
    return messages;
}

void MessageStore::deleteMessage(dpp::snowflake message_id) {
    db.execute(
        "DELETE FROM messages WHERE id = ?",
        {std::to_string(message_id)}
    );
}

void MessageStore::updateUserActivity(dpp::snowflake user_id) {
    db.execute(
        "UPDATE users SET last_seen = ? WHERE id = ?",
        {
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count()),
            std::to_string(user_id)
        }
    );
}

size_t MessageStore::getUserMessageCount(uint64_t user_id, uint64_t guild_id) {
    auto results = db.query(
        "SELECT COUNT(*) FROM messages WHERE author_id = ? AND guild_id = ?",
        {
            std::to_string(user_id),
            std::to_string(guild_id)
        }
    );

    if (results.empty() || results[0].empty()) {
        return 0;
    }

    return std::stoull(results[0][0]);
}

std::vector<std::pair<dpp::snowflake, size_t> > MessageStore::getTopPosters(
    dpp::snowflake guild_id,
    size_t limit
) {
    auto results = db.query(
        "SELECT author_id, COUNT(*) as msg_count "
        "FROM messages "
        "WHERE guild_id = ? "
        "GROUP BY author_id "
        "ORDER BY msg_count DESC "
        "LIMIT ?",
        {
            std::to_string(guild_id),
            std::to_string(limit)
        }
    );

    std::vector<std::pair<dpp::snowflake, size_t> > top_posters;
    top_posters.reserve(results.size());

    for (const auto &row: results) {
        if (row.size() >= 2) {
            top_posters.emplace_back(
                std::stoull(row[0]), // author_id
                std::stoull(row[1]) // message count
            );
        }
    }

    return top_posters;
}

std::vector<std::string> MessageStore::getMostRecentMessages(
    dpp::snowflake guild_id,
    dpp::snowflake user_id,
    size_t limit
) {
    auto results = db.query(
        "SELECT content "
        "FROM messages "
        "WHERE guild_id = ? "
        "AND author_id = ? "
        "AND content != '' "   // Skip empty messages
        "ORDER BY timestamp DESC "  // Newest first
        "LIMIT ?",
        {
            std::to_string(guild_id),
            std::to_string(user_id),
            std::to_string(limit)
        }
    );

    std::vector<std::string> messages;
    messages.reserve(results.size());
    
    // Process results in reverse order to get chronological order (oldest to newest)
    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        const auto& row = *it;
        if (!row.empty()) {
            std::string content = row[0];
            if (!content.empty()) {
                messages.push_back(content);
            }
        }
    }

    return messages;
}

void MessageStore::fetchAndStoreHistory(
    dpp::cluster& cluster,
    dpp::snowflake channel_id,
    dpp::snowflake guild_id,
    size_t& total_messages
) {
    dpp::snowflake before_id = 0;
    bool has_more = true;
    const size_t BATCH_SIZE = 100;

    std::cout << fmt::format("[DEBUG] Starting fetchAndStoreHistory for channel {} in guild {}\n", 
        channel_id, guild_id);

    while (has_more) {
        std::cout << fmt::format("[DEBUG] Fetching batch of {} messages (before_id: {})\n", 
            BATCH_SIZE, before_id);

        auto promise = std::make_shared<std::promise<dpp::message_map>>();
        auto future = promise->get_future();

        std::cout << "[DEBUG] Sending messages_get request to Discord API...\n";
        cluster.messages_get(
            channel_id,
            0,        // around_id
            before_id,// before_id
            0,       // after_id
            BATCH_SIZE,
            [promise](const dpp::confirmation_callback_t& callback) {
                if (callback.is_error()) {
                    std::cout << fmt::format("[ERROR] Discord API error: {}\n", 
                        callback.get_error().message);
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(callback.get_error().message)
                    ));
                    return;
                }
                std::cout << "[DEBUG] Successfully received response from Discord API\n";
                promise->set_value(callback.get<dpp::message_map>());
            }
        );

        std::cout << "[DEBUG] Waiting for future...\n";
        auto messages = future.get();
        
        std::cout << fmt::format("[DEBUG] Received {} messages in this batch\n", 
            messages.size());

        if (messages.empty()) {
            std::cout << "[DEBUG] No more messages to fetch, breaking loop\n";
            has_more = false;
            break;
        }

        std::vector<dpp::message> batch;
        for (const auto& [id, msg] : messages) {
            batch.push_back(msg);
            if (before_id == 0 || id < before_id) {
                before_id = id;
                std::cout << fmt::format("[DEBUG] Updated before_id to {}\n", before_id);
            }
        }

        if (!batch.empty()) {
            std::cout << fmt::format("[DEBUG] Storing batch of {} messages\n", batch.size());
            storeMessages(batch);
            total_messages += batch.size();
            
            std::cout << fmt::format("[DEBUG] Total messages processed so far: {}\n", 
                total_messages);

            if (total_messages % 1000 == 0) {
                std::cout << fmt::format("[INFO] Milestone: Processed {} messages\n", 
                    total_messages);
            }
        }

        std::cout << "[DEBUG] Waiting 250ms before next batch (rate limit protection)\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    std::cout << fmt::format("[DEBUG] Completed fetchAndStoreHistory for channel {}. "
        "Total messages: {}\n", channel_id, total_messages);
}

std::vector<dpp::snowflake> MessageStore::getActiveChannels(dpp::snowflake guild_id, size_t limit) {
    auto results = db.query(
        "SELECT channel_id, COUNT(*) as msg_count "
        "FROM messages "
        "WHERE guild_id = ? "
        "GROUP BY channel_id "
        "ORDER BY msg_count DESC "
        "LIMIT ?",
        {
            std::to_string(guild_id),
            std::to_string(limit)
        }
    );

    std::vector<dpp::snowflake> channels;
    channels.reserve(results.size());

    for (const auto &row: results) {
        if (!row.empty()) {
            channels.push_back(std::stoull(row[0]));
        }
    }

    return channels;
}

std::vector<std::string> MessageStore::searchMessages(
    dpp::snowflake guild_id,
    const std::string &query,
    std::optional<size_t> limit
) {
    std::string sql = fmt::format(
        "SELECT content FROM messages "
        "WHERE guild_id = ? AND content LIKE ? "
        "ORDER BY timestamp DESC{}",
        limit ? fmt::format(" LIMIT {}", *limit) : ""
    );

    auto results = db.query(sql, {
                                std::to_string(guild_id),
                                "%" + query + "%"
                            });

    std::vector<std::string> messages;
    messages.reserve(results.size());

    for (const auto &row: results) {
        if (!row.empty()) {
            messages.push_back(row[0]);
        }
    }

    return messages;
}

std::unordered_map<std::string, size_t> MessageStore::getMessageStats(
    dpp::snowflake guild_id,
    dpp::snowflake user_id,
    std::chrono::system_clock::time_point since
) {
    auto timestamp = since.time_since_epoch().count();

    auto results = db.query(
        "SELECT "
        "COUNT(*) as total_messages, "
        "AVG(LENGTH(content)) as avg_length, "
        "COUNT(DISTINCT channel_id) as channels_used, "
        "COUNT(CASE WHEN attachments != '[]' THEN 1 END) as messages_with_attachments, "
        "COUNT(CASE WHEN reactions != '[]' THEN 1 END) as messages_with_reactions "
        "FROM messages "
        "WHERE guild_id = ? AND author_id = ? AND timestamp >= ?",
        {
            std::to_string(guild_id),
            std::to_string(user_id),
            std::to_string(timestamp)
        }
    );

    std::unordered_map<std::string, size_t> stats;
    if (!results.empty() && results[0].size() >= 5) {
        stats["total_messages"] = std::stoull(results[0][0]);
        stats["avg_message_length"] = std::stoull(results[0][1]);
        stats["channels_used"] = std::stoull(results[0][2]);
        stats["messages_with_attachments"] = std::stoull(results[0][3]);
        stats["messages_with_reactions"] = std::stoull(results[0][4]);
    }

    return stats;
}

std::vector<GuildMessage> MessageStore::getRecentGuildMessages(
    dpp::snowflake guild_id,
    size_t limit
) {
    auto results = db.query(
        "SELECT m.id, m.author_id, u.username, m.content, m.timestamp, "
        "m.channel_id, m.pinned, m.attachments, m.reactions, m.referenced_message_id "
        "FROM messages m "
        "LEFT JOIN users u ON m.author_id = u.id "
        "WHERE m.guild_id = ? "
        "AND m.content != '' "   // Skip empty messages
        "ORDER BY m.timestamp DESC "  // Newest first
        "LIMIT ?",
        {
            std::to_string(guild_id),
            std::to_string(limit)
        }
    );

    std::vector<GuildMessage> messages;
    messages.reserve(results.size());
    
    // Process results in reverse order to get chronological order (oldest to newest)
    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        const auto& row = *it;
        if (row.size() >= 10) {  // Make sure we have all fields
            GuildMessage msg;
            msg.id = std::stoull(row[0]);
            msg.author_id = std::stoull(row[1]);
            msg.author_name = row[2];
            msg.content = row[3];
            msg.timestamp = std::stoull(row[4]);
            msg.channel_id = std::stoull(row[5]);
            msg.pinned = (row[6] == "1");
            msg.attachments = row[7];
            msg.reactions = row[8];
            if (!row[9].empty()) {
                msg.referenced_message_id = std::stoull(row[9]);
            }
            messages.push_back(msg);
        }
    }

    return messages;
}

void MessageStore::clearGuildMessages(dpp::snowflake guild_id) {
    db.execute(
        "DELETE FROM messages WHERE guild_id = ?",
        {std::to_string(guild_id)}
    );
}

std::optional<MessageInfo> MessageStore::getMessageById(dpp::snowflake message_id) {
	std::cout << "Looking up message ID: " << message_id << std::endl;  // Debug log

	auto results = db.query(
		"SELECT m.content, m.author_id, u.username "
		"FROM messages m "
		"LEFT JOIN users u ON m.author_id = u.id "
		"WHERE m.id = ?",
		{std::to_string(message_id)}
	);

	std::cout << "Query executed. Found " << results.size() << " results" << std::endl;  // Debug log

	if (results.empty() || results[0].empty()) {
		std::cout << "No results found for message ID: " << message_id << std::endl;  // Debug log
		return std::nullopt;
	}

	// Debug log the values we found
	std::cout << "Found message: " << std::endl
			  << "Content: " << results[0][0] << std::endl
			  << "Author ID: " << results[0][1] << std::endl
			  << "Username: " << results[0][2] << std::endl;

	MessageInfo info;
	info.content = results[0][0];
	info.author_id = std::stoull(results[0][1]);
	info.author_name = results[0][2];

	return info;
}

void MessageStore::syncGuildMessages(
    dpp::cluster& cluster,
    dpp::snowflake guild_id,
    const std::function<void(const std::string&, size_t, size_t)>& progress_callback
) {
    std::cout << fmt::format("[DEBUG] Starting guild sync for guild {}\n", guild_id);
    
    // Start transaction
    std::cout << "[DEBUG] Beginning database transaction\n";

    try {
        size_t total_messages = 0;
        auto channels = cluster.channels_get_sync(guild_id);
        
        std::cout << fmt::format("[DEBUG] Found {} channels in guild\n", channels.size());
        
        // Count text channels first
        size_t text_channel_count = 0;
        for (const auto& [_, channel] : channels) {
            if (channel.is_text_channel()) {
                text_channel_count++;
            }
        }
        
        std::cout << fmt::format("[DEBUG] Found {} text channels to process\n", text_channel_count);
        
        // Process each channel
        size_t processed_channels = 0;
        for (const auto& [_, channel] : channels) {
            if (channel.is_text_channel()) {
                processed_channels++;
                std::cout << fmt::format("[DEBUG] Processing channel #{} ({}/{} text channels)\n", 
                    channel.name, processed_channels, text_channel_count);
                
                try {
                    size_t channel_messages_before = total_messages;
                    fetchAndStoreHistory(cluster, channel.id, guild_id, total_messages);
                    
                    std::cout << fmt::format("[DEBUG] Channel #{} processed. Added {} new messages\n", 
                        channel.name, total_messages - channel_messages_before);
                    
                    // Update progress through callback
                    progress_callback(channel.name, processed_channels, text_channel_count);
                    
                } catch (const std::exception& e) {
                    std::cout << fmt::format("[ERROR] Failed processing channel #{}: {}\n", 
                        channel.name, e.what());
                    // Continue with next channel instead of breaking
                }
            }
        }
        
        std::cout << "[DEBUG] Committing transaction\n";
        db.commit();
        std::cout << fmt::format("[DEBUG] Transaction committed successfully. Total messages: {}\n", 
            total_messages);
        
    } catch (const std::exception& e) {
        std::cout << fmt::format("[ERROR] Error during processing: {}. Rolling back...\n", e.what());
        db.rollback();
        throw;
    }
}

std::vector<std::string> MessageStore::getMostRecentMessagesWithContent(
    dpp::snowflake guild_id,
    dpp::snowflake user_id,
    size_t limit
) {
    auto results = db.query(
        "SELECT content "
        "FROM messages "
        "WHERE guild_id = ? "
        "AND author_id = ? "
        "AND content IS NOT NULL "  // Ensure content is not null
        "AND content != '' "        // Ensure content is not empty
        "AND trim(content) != '' "  // Ensure content is not just whitespace
        "ORDER BY timestamp DESC "  // Newest first
        "LIMIT ?",
        {
            std::to_string(guild_id),
            std::to_string(user_id),
            std::to_string(limit)
        }
    );

    std::vector<std::string> messages;
    messages.reserve(results.size());
    
    // Process results in reverse order to get chronological order (oldest to newest)
    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        const auto& row = *it;
        if (!row.empty()) {
            std::string content = row[0];
            if (!content.empty()) {
                messages.push_back(content);
            }
        }
    }

    return messages;
}