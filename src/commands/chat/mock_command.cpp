#include "commands/chat/mock_command.h"
#include <thread>
#include <utils/misc/cringe_environment.h>
#include "utils/embed/cringe_embed.h"
#include "utils/misc/cringe_emoji.h"

dpp::slashcommand mock_declaration() {
    return dpp::slashcommand()
        .set_name("mock")
        .set_description("Roast a user based on their message history")
        .add_option(dpp::command_option(dpp::co_user, "user", "User to mock", true));
}

std::string formatMessages(const std::vector<std::string>& messages) {
	std::vector<std::string> formattedMessages;
	formattedMessages.reserve(messages.size());

	for (size_t i = 0; i < messages.size(); ++i) {
		formattedMessages.push_back(fmt::format("Message {}: {}", i + 1, messages[i]));
	}

	return fmt::format("{}", fmt::join(formattedMessages, "\n"));
}

void mock_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking();

    try {
		auto target_user = std::get<dpp::snowflake>(event.get_parameter("user"));
    	const std::string sys_prompt = get_env("SYSTEM_ROLE");

        std::thread([&cringe, event, target_user, sys_prompt]() {
            try {
                // Loading embed
                auto loading_embed = CringeEmbed::createLoading("Analyzing Target", "")
                    .setDescription(
                        fmt::format(
                            "{}   Gathering evidence\n"
                            "{}   Analyzing cringe levels\n"
                            "{}   Generating roast\n"
                            "{}   Finalizing destruction",
                            CringeEmoji::spinner,
                            CringeEmoji::spinner,
                            CringeEmoji::spinner,
                            CringeEmoji::spinner
                        )
                    )
                    .setFooter("Step 1 of 4", cringe.cluster.me.get_avatar_url());

                event.edit_original_response(dpp::message(
                    event.command.channel_id, loading_embed.build()
                ));

                // Get user's messages
                auto messages = cringe.message_store->getMostRecentMessagesWithContent(
                    event.command.guild_id,
                    target_user,
                    100
                );

                if (messages.empty()) {
                    auto error_embed = CringeEmbed::createError(
                        fmt::format(
                            "{}   Gathering messages\n"
                            "   → Error: No messages found for this user!\n"
                            "{}   Analyzing cringe levels\n"
                            "{}   Generating roast\n"
                            "{}   Finalizing destruction",
                            CringeEmoji::error,
                            CringeEmoji::error,
                            CringeEmoji::error,
                            CringeEmoji::error
                        )
                    )
                    .setTitle("Request Failed")
                	.setFooter("Couldn't generate message", CringeIcon::ErrorIcon);
                    
                    event.edit_original_response(dpp::message(
                        event.command.channel_id, error_embed.build()
                    ));
                    return;
                }

                // Update embed after messages gathered
                auto processing_embed = loading_embed
                    .setDescription(
                        fmt::format(
                            "{}   Gathering messages\n"
                            "{}   Analyzing cringe levels\n"
                            "{}   Generating roast\n"
                            "{}   Finalizing destruction",
                            CringeEmoji::success,
                            CringeEmoji::success,
                            CringeEmoji::spinner,
                            CringeEmoji::spinner
                        )
                    ).setFooter("Step 3 of 4", cringe.cluster.me.get_avatar_url());

                event.edit_original_response(dpp::message(
                    event.command.channel_id, processing_embed.build()
                ));

                // Build the AI prompt
                std::string ai_prompt = fmt::format(
                "Using the same tone as above, you are tasked with creating a brutal roast of a discord user based on {0} messages from their message history. "
					"You are speaking TO the user, not AS them. "
					"Below are {0} messages from the user you're roasting:\n\n{1}\n\n"
					"Create a lethal and hilarious roast that directly addresses the user '{2}', mentioned as '<{3}>'. "
					"1. Make fun of their interests, typing habits, and discussion themes\n"
					"2. Cite specific instances from their conversations\n"
					"3. Simultaneously evoke brutal savagery and hilarity\n"
					"4. Remain within the word limit of 150-250 words\n\n"
                    "IMPORTANT: Generate only the roast message. Do not include any explanations or additional commentary!!!",
                    messages.size(),
                    formatMessages(messages),
                    event.command.get_resolved_user(target_user).username,
                    event.command.get_resolved_user(target_user).get_mention()
                );

                // Update embed before AI call
                processing_embed.setDescription(
                    fmt::format(
                        "{}   Gathering messages\n"
                        "{}   Analyzing cringe levels\n"
                        "{}   Generating mock\n"
                        "{}   Finalizing mock",
                        CringeEmoji::success,
                        CringeEmoji::success,
                        CringeEmoji::spinner,
                        CringeEmoji::spinner
                    )).setFooter("Step 3 of 4", cringe.cluster.me.get_avatar_url());

                event.edit_original_response(dpp::message(
                    event.command.channel_id, processing_embed.build()
                ));

                // Get AI response
                json ollama_response;
                try {
                    ollama_response = cringe.ollama.chat("cringe", ai_prompt, sys_prompt);
                } catch (const std::exception& e) {
                    auto error_embed = CringeEmbed::createError(
                        fmt::format(
                            "{}   Gathering messages\n"
                            "{}   Analyzing cringe levels\n"
                            "{}   Generating mock\n"
                            "\t\t → Error: {}\n"
                            "{}   Finalizing mock",
                            CringeEmoji::success,
                            CringeEmoji::success,
                            CringeEmoji::error,
                            e.what(),
                            CringeEmoji::error
                        )
                    ).setTitle("Request Failed")
                	.setFooter("Couldn't generate message", CringeIcon::ErrorIcon);
                    
                    event.edit_original_response(dpp::message(
                        event.command.channel_id, error_embed.build()
                    ));
                    return;
                }

                std::string roast = ollama_response["response"];

                // Get user info for the embed
                cringe.cluster.user_get(target_user, [event, roast, &cringe](
                    const dpp::confirmation_callback_t& cb) {
                    if (cb.is_error()) {
                        auto error_embed = CringeEmbed::createError(
                            "Failed to fetch user information"
                        ).setTitle("Request Failed");
                        
                        event.edit_original_response(dpp::message(
                            event.command.channel_id, error_embed.build()
                        ));
                        return;
                    }

                    auto user = std::get<dpp::user_identified>(cb.value);
                    
                    // Replace placeholders with actual user info
                    std::string formatted_roast = roast;
                    size_t username_pos = formatted_roast.find("{{username}}");
                    while (username_pos != std::string::npos) {
                        formatted_roast.replace(username_pos, 12, user.username);
                        username_pos = formatted_roast.find("{{username}}");
                    }

                    size_t mention_pos = formatted_roast.find("{{mention}}");
                    while (mention_pos != std::string::npos) {
                        formatted_roast.replace(mention_pos, 11, user.get_mention());
                        mention_pos = formatted_roast.find("{{mention}}");
                    }
                    
                    auto final_embed = CringeEmbed()
                        .setTitle(fmt::format("Mocking {}", user.username))
                        .setDescription(
                            fmt::format(
                                "**Process Complete**\n"
                                "{}   Gathering messages\n"
                                "{}   Analyzing cringe levels\n"
                                "{}   Generating mock\n"
                                "{}   Finalizing mock\n\n"
                                "**Output:**\n{}",
                                CringeEmoji::success,
                                CringeEmoji::success,
                                CringeEmoji::success,
                                CringeEmoji::success,
                                formatted_roast
                            )
                        )
                        .setFooter("Mock a user with /mock!", cringe.cluster.me.get_avatar_url());
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