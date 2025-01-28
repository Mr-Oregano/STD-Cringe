#include "commands/test/sync_command.h"
#include <mutex>
#include <thread>
#include "utils/embed/cringe_embed.h"

static std::mutex sync_mutex;

struct SyncStats {
	size_t messages_seen = 0;
	size_t messages_saved = 0;
	size_t channels_processed = 0;
	size_t total_channels = 0;
	std::string current_channel = "";
};

dpp::slashcommand sync_declaration() {
	return dpp::slashcommand()
			.set_name("sync")
			.set_description("Sync server data to database")
			.add_option(dpp::command_option(dpp::co_sub_command, "messages", "Sync all server messages"))
			.add_option(dpp::command_option(dpp::co_sub_command, "users", "Sync all server users"));
}

void processMessages(CringeBot &cringe, const dpp::message_map &message_map, std::shared_ptr<SyncStats> stats,
                     dpp::snowflake guild_id) {
    cringe.logger.log(LogLevel::INFO, fmt::format(
        "Processing batch of {} messages for guild {}",
        message_map.size(),
        guild_id
    ));

    std::vector<dpp::message> valid_messages;

    for (const auto &[_, msg]: message_map) {
        stats->messages_seen++;
        valid_messages.push_back(msg);  // Store ALL messages
        stats->messages_saved++;

        if (msg.content.empty() && !msg.attachments.empty()) {
            cringe.logger.log(LogLevel::DEBUG, fmt::format(
                "Storing message {} with {} attachments but no text content",
                msg.id,
                msg.attachments.size()
            ));
        }
    }

    if (!valid_messages.empty()) {
        cringe.logger.log(LogLevel::INFO, fmt::format(
            "Storing {} messages in database for guild {}",
            valid_messages.size(),
            guild_id
        ));
        
        try {
            cringe.message_store->storeMessages(valid_messages);
            cringe.logger.log(LogLevel::INFO, "Messages stored successfully");
        } catch (const std::exception& e) {
            cringe.logger.log(LogLevel::ERROR, fmt::format(
                "Failed to store messages: {}",
                e.what()
            ));
            throw;
        }
    }
}

void fetchChannelMessages(CringeBot &cringe, const dpp::channel &channel, std::shared_ptr<SyncStats> stats) {
    cringe.logger.log(LogLevel::INFO, fmt::format("Starting to fetch messages from #{}", channel.name));
    
    dpp::snowflake last_id = 0;
    bool has_more = true;
    const size_t BATCH_SIZE = 100;

    while (has_more) {
        try {
            auto promise = std::make_shared<std::promise<dpp::message_map>>();
            auto future = promise->get_future();

            cringe.cluster.messages_get(
                channel.id,
                0,       // around
                last_id, // before
                0,       // after
                BATCH_SIZE,
                [promise](const dpp::confirmation_callback_t &cb) {
                    if (cb.is_error()) {
                        promise->set_exception(std::make_exception_ptr(
                            std::runtime_error(cb.get_error().message)));
                        return;
                    }
                    promise->set_value(std::get<dpp::message_map>(cb.value));
                }
            );

            dpp::message_map messages = future.get();
            
            if (messages.empty()) {
                has_more = false;
                break;
            }

            // Find oldest message ID
            for (const auto &[id, _] : messages) {
                if (last_id == 0 || id < last_id) {
                    last_id = id;
                }
            }

            // Store messages
            std::vector<dpp::message> batch;
            for (const auto &[_, msg] : messages) {
                batch.push_back(msg);
                stats->messages_seen++;
            }

            cringe.message_store->storeMessages(batch);
            stats->messages_saved += batch.size();

            cringe.logger.log(LogLevel::DEBUG, fmt::format(
                "Processed batch of {} messages in #{}", batch.size(), channel.name
            ));

            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Rate limiting
        } catch (const std::exception &e) {
            cringe.logger.log(LogLevel::ERROR, fmt::format(
                "Error fetching messages from #{}: {}", channel.name, e.what()
            ));
            break;
        }
    }
}

void sync_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    std::cout << "[DEBUG] Starting sync command\n";

    if (event.command.usr.id != 933796468731568191) {
        std::cout << fmt::format("[DEBUG] Unauthorized access attempt by user {}\n",
            event.command.usr.id);
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError("You are not authorized to use this command!").build()
        ));
        return;
    }

    std::cout << "[DEBUG] Authorization check passed\n";
    event.thinking(true);

    try {
        auto cmd_data = event.command.get_command_interaction();
        auto &subcommand = cmd_data.options[0];

        if (subcommand.name == "messages") {
            std::cout << "[DEBUG] Processing 'messages' subcommand\n";

            auto thread_cringe = &cringe;
            auto thread_event = std::make_shared<dpp::slashcommand_t>(event);
            auto guild_id = event.command.guild_id;

            std::cout << fmt::format("[DEBUG] Starting sync for guild {}\n", guild_id);

            std::thread([thread_cringe, thread_event, guild_id]() {
                try {
                    std::cout << "[DEBUG] Acquiring sync mutex\n";
                    std::lock_guard<std::mutex> lock(sync_mutex);

                    thread_cringe->logger.log(LogLevel::INFO, "STARTING TO EXTRACT");

                    // Create progress callback
                    auto progress_callback = [thread_event, thread_cringe](
                        const std::string& channel_name,
                        size_t processed_channels,
                        size_t total_channels
                    ) {
                        auto progress_embed = CringeEmbed::createLoading(
                            "Syncing Messages",
                            fmt::format(
                                "Processing channel: #{}\n"
                                "Progress: {}/{} channels",
                                channel_name,
                                processed_channels,
                                total_channels
                            )
                        );

                        thread_event->edit_original_response(dpp::message(
                            thread_event->command.channel_id,
                            progress_embed.build()
                        ));
                    };

                    // Perform the sync
                    thread_cringe->message_store->syncGuildMessages(
                        thread_cringe->cluster,
                        guild_id,
                        progress_callback
                    );

                    thread_cringe->logger.log(LogLevel::INFO, "done!");

                    auto success = CringeEmbed::createSuccess("Sync Complete! 🎉")
                        .setDescription("Successfully synced all messages across all channels!");

                    thread_event->edit_original_response(dpp::message(
                        thread_event->command.channel_id,
                        success.build()
                    ));

                } catch (const std::exception &e) {
                    std::cout << fmt::format("[ERROR] Sync failed: {}\n", e.what());
                    thread_event->edit_original_response(dpp::message(
                        thread_event->command.channel_id,
                        CringeEmbed::createError(fmt::format("Sync failed: {}", e.what())).build()
                    ));
                }
            }).detach();
        }
    } catch (const std::exception &e) {
        std::cout << fmt::format("[ERROR] Command processing failed: {}\n", e.what());
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError(e.what()).build()
        ));
    }
}