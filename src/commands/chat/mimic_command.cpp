#include "commands/chat/mimic_command.h"
#include "utils/embed/cringe_embed.h"
#include <thread>

dpp::slashcommand mimic_declaration() {
    return dpp::slashcommand()
        .set_name("mimic")
        .set_description("Mock another user's writing style")
        .add_option(dpp::command_option(dpp::co_user, "user", "User to mock", true));
}

void mimic_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking();

    try {
        dpp::snowflake target_user = std::get<dpp::snowflake>(event.get_parameter("user"));
        
        std::thread([&cringe, event, target_user]() {
            try {
                // Loading embed
                auto loading_embed = CringeEmbed::createLoading("Analyzing User Communication", "")
                    .setDescription(
                        "1. Gathering user messages ⏳\n"
                        "2. Analyzing message patterns ⌛\n"
                        "3. Generating mimicked response ⌛\n"
                        "4. Finalizing results ⌛"
                    )
                    .setFooter("Please wait while I analyze the user's communication style...");

                event.edit_original_response(dpp::message(
                    event.command.channel_id, loading_embed.build()
                ));

                // Step 1: Get user's messages
                auto messages = cringe.message_store->getMostRecentMessages(
                    event.command.guild_id,
                    target_user,
                    100
                );

                if (messages.empty()) {
                    auto error_embed = CringeEmbed::createError(
                        "1. Gathering user messages ❌\n"
                        "   → Error: No messages found for this user!\n"
                        "2. Analyzing message patterns ⌛\n"
                        "3. Generating mimicked response ⌛\n"
                        "4. Finalizing results ⌛"
                    )
                    .setTitle("Analysis Failed");
                    
                    event.edit_original_response(dpp::message(
                        event.command.channel_id, error_embed.build()
                    ));
                    return;
                }

                // Update embed after messages gathered
                auto processing_embed = loading_embed
                    .setDescription(
                        "1. Gathering user messages ✅\n"
                        "2. Analyzing message patterns ⏳\n"
                        "3. Generating mimicked response ⌛\n"
                        "4. Finalizing results ⌛"
                    );

                event.edit_original_response(dpp::message(
                    event.command.channel_id, processing_embed.build()
                ));

                // Build the AI prompt
                std::string ai_prompt = fmt::format(
                "You are tasked with generating a new message that mimics the communication style of a specific Discord user. "
					"Below are their {0} most recent messages:\n\n{1}\n\n"
					"Your task:\n"
					"1. Analyze the user's messages to identify:\n"
					"   - Topics they frequently discuss.\n"
					"   - Their vocabulary, tone, and word choices.\n"
					"   - Sentence structure and flow.\n"
					"   - Typical emoji and punctuation usage.\n"
					"   - The average length of their messages.\n"
					"2. Write a single, new message that:\n"
					"   - Naturally continues their usual conversations.\n"
					"   - Accurately reflects their writing style, vocabulary, and tone.\n"
					"   - Matches their typical message length.\n"
					"   - Appropriately uses emojis and punctuation ONLY as they do.\n"
					"   - Focuses on topics they've shown interest in.\n\n"
					"Important: Generate only the new message without any explanation or commentary. "
					"The message must appear as though it was written by them."
					"Remember to stay true to their style.",
                    messages.size(),
                    fmt::join(messages, "\n")
                );

                // Update embed before AI call
                processing_embed.setDescription(
                    "1. Gathering user messages ✅\n"
                    "2. Analyzing message patterns ✅\n"
                    "3. Generating mimicked response ⏳\n"
                    "4. Finalizing results ⌛"
                );

                event.edit_original_response(dpp::message(
                    event.command.channel_id, processing_embed.build()
                ));

                // Step 3: Get AI response
                json ollama_response;
                try {
                    ollama_response = cringe.ollama.chat("dolphin-llama3", ai_prompt);
                } catch (const std::exception& e) {
                    auto error_embed = CringeEmbed::createError(
                        fmt::format(
                            "1. Gathering user messages ✅\n"
                            "2. Analyzing message patterns ✅\n"
                            "3. Generating mimicked response ❌\n"
                            "   → Error: {}\n"
                            "4. Finalizing results ⌛",
                            e.what()
                        )
                    ).setTitle("Analysis Failed");
                    
                    event.edit_original_response(dpp::message(
                        event.command.channel_id, error_embed.build()
                    ));
                    return;
                }
                std::string mocked_response = ollama_response["response"];

                // Update embed before final step
                processing_embed.setDescription(
                    "1. Gathering user messages ✅\n"
                    "2. Analyzing message patterns ✅\n"
                    "3. Generating mimicked response ✅\n"
                    "4. Finalizing results ⏳"
                );

                event.edit_original_response(dpp::message(
                    event.command.channel_id, processing_embed.build()
                ));

                // Get user info for the embed
                cringe.cluster.user_get(target_user, [event, mocked_response](
                    const dpp::confirmation_callback_t& cb) {
                    if (cb.is_error()) {
                        auto error_embed = CringeEmbed::createError(
                            "1. Gathering user messages ✅\n"
                            "2. Analyzing message patterns ✅\n"
                            "3. Generating mimicked response ✅\n"
                            "4. Finalizing results ❌\n"
                            "   → Error: Failed to fetch user information"
                        ).setTitle("Analysis Failed");
                        
                        event.edit_original_response(dpp::message(
                            event.command.channel_id, error_embed.build()
                        ));
                        return;
                    }

                    auto user = std::get<dpp::user_identified>(cb.value);
                    
                    auto final_embed = CringeEmbed()
                        .setTitle(fmt::format("Mimicking {}'s Style", user.username))
                        .setDescription(
                            fmt::format(
                                "**Process Complete:**\n"
                                "1. Gathering user messages ✅\n"
                                "2. Analyzing message patterns ✅\n"
                                "3. Generating mimicked response ✅\n"
                                "4. Finalizing results ✅\n\n"
                                "**Generated Message:**\n{}",
                                mocked_response
                            )
                        )
                        .setFooter(fmt::format(
                            "Requested by {} • Mimicking {}",
                            event.command.usr.username,
                            user.username
                        ))
                        .setThumbnail(user.get_avatar_url());

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, 
                        final_embed.build()
                    ));
                });

            } catch (const std::exception &e) {
                auto error_embed = CringeEmbed::createError(
                    fmt::format(
                        "An unexpected error occurred:\n{}", 
                        e.what()
                    )
                ).setTitle("Process Failed");
                
                event.edit_original_response(dpp::message(
                    event.command.channel_id,
                    error_embed.build()
                ));
            }
        }).detach();

    } catch (const std::exception &e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError(e.what()).build()
        ));
    }
}