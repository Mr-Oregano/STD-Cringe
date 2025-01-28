#include "commands/chat/memory_command.h"
#include "utils/embed/cringe_embed.h"
#include <future>
#include <thread>

dpp::slashcommand memory_declaration() {
    return dpp::slashcommand()
        .set_name("memory")
        .set_description("Analyze a user's personality based on their messages")
        .add_option(dpp::command_option(dpp::co_user, "user", 
            "User to analyze", true))
        .add_option(dpp::command_option(dpp::co_integer, "messages",
            "Number of messages to analyze (max 1000)", false));
}

void memory_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(false);

    try {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();
        dpp::snowflake user = cmd_data.get_value<dpp::snowflake>(0);
        int64_t message_count = cmd_data.options.size() > 1 ? 
            cmd_data.get_value<int64_t>(1) : 100;
        
        message_count = std::min(static_cast<int64_t>(1000), message_count);

        std::thread([&cringe, event, user, message_count]() {
            try {
                std::vector<std::string> messages;
                auto channel = event.command.channel_id;
                
                // Get user info first
                cringe.cluster.user_get(user, [&cringe, event, &messages, message_count, channel](
                    const dpp::confirmation_callback_t& user_cb) {
                    if (user_cb.is_error()) {
                        event.edit_original_response(dpp::message(
                            event.command.channel_id,
                            CringeEmbed::createError("Could not find user").build()
                        ));
                        return;
                    }

                    auto user = std::get<dpp::user_identified>(user_cb.value);
                    
                    // Get messages
                    cringe.cluster.messages_get(channel, 0, 0, 0, message_count,
                        [user, event, &messages, &cringe](const dpp::confirmation_callback_t& msg_cb) {
                            if (msg_cb.is_error()) {
                                event.edit_original_response(dpp::message(
                                    event.command.channel_id,
                                    CringeEmbed::createError("Failed to fetch messages").build()
                                ));
                                return;
                            }

                            const auto& message_map = std::get<dpp::message_map>(msg_cb.value);
                            for (const auto& [_, msg] : message_map) {
                                if (msg.author.id == user.id) {
                                    messages.push_back(msg.content);
                                }
                            }

                            if (messages.empty()) {
                                event.edit_original_response(dpp::message(
                                    event.command.channel_id,
                                    CringeEmbed::createError("No messages found for this user").build()
                                ));
                                return;
                            }

                            // Analyze messages
                            std::string combined = fmt::format(
                                "Analyze these messages and create a fun personality profile: {}", 
                                fmt::join(messages, " | ")
                            );

                            json ollama_response = cringe.ollama.chat(combined, "cringe");
                            std::string analysis = ollama_response["response"];

                            // Create response embed
                            auto embed = CringeEmbed()
                                .setTitle(fmt::format("Memory Analysis: @{}", user.username))
                                .setDescription(analysis)
                                .addField("Messages Analyzed", std::to_string(messages.size()), true)
                                .addField("User", user.get_mention(), true)
                                .setFooter("analyze user messages with /memory!");

                            event.edit_original_response(dpp::message(
                                event.command.channel_id, embed.build()
                            ));
                        }
                    );
                });

            } catch (const std::exception &e) {
                event.edit_original_response(dpp::message(
                    event.command.channel_id,
                    fmt::format("Error processing command: {}", e.what())
                ));
            }
        }).detach();

    } catch (const std::exception &e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            fmt::format("Error starting command: {}", e.what())
        ));
    }
}