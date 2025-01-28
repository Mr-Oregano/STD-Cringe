#include "commands/chat/jinx_command.h"
#include <dpp/unicode_emoji.h>
#include <random>
#include <thread>
#include <utils/misc/cringe_environment.h>

#include "utils/embed/cringe_embed.h"

dpp::slashcommand jinx_declaration() {
    return dpp::slashcommand()
        .set_name("jinx")
        .set_description("Jinx a user with annoying effects")
        .add_option(dpp::command_option(dpp::co_user, "target", "User to jinx", true))
        .add_option(dpp::command_option(dpp::co_integer, "duration", "Duration in minutes (max 10)", true))
        .add_option(dpp::command_option(dpp::co_boolean, "subtle", "Make effects more subtle", false));
}

void jinx_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking();

    try {
        auto target_user = std::get<dpp::snowflake>(event.get_parameter("target"));
        auto duration = std::min(std::get<int64_t>(event.get_parameter("duration")), static_cast<int64_t>(10));
        bool subtle = event.get_parameter("subtle").index() ?
            std::get<bool>(event.get_parameter("subtle")) : false;

        // Store jinx state in bot
        cringe.jinxed_users[static_cast<uint64_t>(target_user)] = {
            .end_time = std::chrono::system_clock::now() + std::chrono::minutes(duration),
            .subtle = subtle
        };

        // Add message create handler for jinxed user
        cringe.cluster.on_message_create([&cringe, target_user](const dpp::message_create_t& msg_event) {
        	auto jinx_it = cringe.jinxed_users.find(target_user);
			if (msg_event.msg.author.id != target_user || jinx_it == cringe.jinxed_users.end()) {
				return;
			}

			// Check if jinx has expired
			if (std::chrono::system_clock::now() > jinx_it->second.end_time) {
				// Remove expired jinx
				cringe.jinxed_users.erase(jinx_it);
				return;
			}

            std::random_device rd;
            std::mt19937 gen(rd());

        	for (auto [fst, snd] : cringe.jinxed_users) {
        		std::cout << fst << "\n" << std::endl;
        	}

			// Random chance to apply effects
            if (std::uniform_real_distribution<> dis(0.0, 1.0);
				dis(gen) < (cringe.jinxed_users[target_user].subtle ? 0.3 : 0.6)) {
                // Add clown reaction
            	cringe.cluster.message_add_reaction(msg_event.msg, dpp::unicode_emoji::clown_face);
            	const auto system_prompt = get_env("SYSTEM_PROMPT").substr(0, 500);

                // Generate AI correction/completion
				const std::string ai_prompt = fmt::format(
                    "Using the same tone as above, you must correct/complete a Discord message in a very funny, crude, and humourous way.\n"
                    "Original message: {}\n\n"
                    "IMPORTANT: Choose ONLY ONE of these responses:\n"
                    "1. Correct their grammar/spelling in a condescending way\n"
                    "2. Add 'uwu' speak modifications\n"
                    "3. Add a stutter effect\n"
                    "4. Complete their thought in an unexpected way\n\n"
                    "Keep the response short and funny. Don't explain which option you chose.",
                    msg_event.msg.content
                );

                try {
                    json response = cringe.openai.chat("gpt-3.5-turbo", ai_prompt, system_prompt.substr(0, 150));
					const std::string correction = response["response"];

                    dpp::message reply(msg_event.msg.channel_id, correction);
                    reply.set_reference(msg_event.msg.id);
                    cringe.cluster.message_create(reply);
                } catch (...) {
                    // Catch error and log
                    std::cout << "Error generating AI response" << std::endl;
                }
            }
        });

        auto success_embed = CringeEmbed::createSuccess(
            fmt::format("Successfully jinxed {} for {} minutes!",
                event.command.get_resolved_user(target_user).username,
                duration
            )
        );
        event.edit_original_response(dpp::message(
            event.command.channel_id, success_embed.build()
        ));

    } catch (const std::exception& e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError(e.what()).build()
        ));
    }
}