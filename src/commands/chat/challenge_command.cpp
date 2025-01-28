#include "commands/chat/challenge_command.h"
#include "utils/embed/cringe_embed.h"
#include "utils/embed/challenge_embed.h"
#include "utils/bot/cringe_bot.h"
#include <fmt/format.h>

dpp::slashcommand challenge_declaration() {
    return dpp::slashcommand()
        .set_name("challenge")
        .set_description("Coding challenges")
        .add_option(
            dpp::command_option(dpp::co_sub_command, "view", "View available challenges")
        )
        .add_option(
            dpp::command_option(dpp::co_sub_command, "show", "Show a specific challenge")
            .add_option(
                dpp::command_option(dpp::co_integer, "challenge_id", "ID of the challenge", true)
            )
        )
        .add_option(
            dpp::command_option(dpp::co_sub_command, "leaderboard", "View leaderboard")
            .add_option(
                dpp::command_option(dpp::co_string, "scope", "Leaderboard scope", true)
                .add_choice(dpp::command_option_choice("Server", std::string("server")))
                .add_choice(dpp::command_option_choice("Global", std::string("global")))
            )
        )
        .add_option(
            dpp::command_option(dpp::co_sub_command, "stats", "View your challenge statistics")
        );
}

void challenge_command(CringeBot& cringe, const dpp::slashcommand_t& event) {
    event.thinking(true);

    try {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();
        auto& subcommand = cmd_data.options[0];

        if (subcommand.name == "view") {
            auto challenges = cringe.challenge_store->getAllChallenges();
            
            // Create paginated challenge list
            std::vector<dpp::embed> pages;
            const size_t CHALLENGES_PER_PAGE = 5;
            
            for (size_t i = 0; i < challenges.size(); i += CHALLENGES_PER_PAGE) {
                auto page_embed = CringeEmbed()
                    .setTitle("🎯 Coding Challenges")
                    .setDescription(fmt::format("Page {}/{}", 
                        (i / CHALLENGES_PER_PAGE) + 1,
                        (challenges.size() + CHALLENGES_PER_PAGE - 1) / CHALLENGES_PER_PAGE));

                // Add challenges for this page
                for (size_t j = i; j < std::min(i + CHALLENGES_PER_PAGE, challenges.size()); j++) {
                    const auto& challenge = challenges[j];
                    std::string field_value = fmt::format(
                        "{}\n{}",
                        challenge.description,
                        ChallengeEmbed::formatPoints(challenge)
                    );
                    
                    page_embed.addField(
                        fmt::format("#{} {}", challenge.challenge_id, challenge.title),
                        field_value,
                        false
                    );
                }
                
                pages.push_back(page_embed.build());
            }

            // Create message with first page and components
            dpp::message msg(event.command.channel_id, pages[0]);
            msg.add_component(ChallengeEmbed::createChallengeListRow(challenges));

            // Send paginated embed
            CringeEmbed().sendPaginatedEmbed(
                pages,
                event.command.channel_id,
                cringe.cluster,
                *cringe.embed_store
            );

        } else if (subcommand.name == "show") {
            auto challenge_id = subcommand.get_value<int64_t>(0);
            auto challenge_opt = cringe.challenge_store->getChallenge(challenge_id);

            if (!challenge_opt) {
                event.edit_original_response(dpp::message(
                    event.command.channel_id,
                    CringeEmbed::createError("Challenge not found!").build()
                ));
                return;
            }

            const auto& challenge = *challenge_opt;
            auto embed = ChallengeEmbed::createChallengeDetailEmbed(
                challenge,
                event.command.usr.id,
                *cringe.challenge_store
            );

            dpp::message msg(event.command.channel_id, embed.build());
            msg.add_component(ChallengeEmbed::createChallengeDetailRow(challenge, 1));
            event.edit_original_response(msg);

        } else if (subcommand.name == "leaderboard") {
            auto scope = subcommand.get_value<std::string>(0);
            bool is_global = (scope == "global");
            
            auto leaderboard = cringe.challenge_store->getLeaderboard(
                is_global,
                event.command.guild_id
            );

            auto embed = CringeEmbed()
                .setTitle(fmt::format("🏆 {} Leaderboard", is_global ? "Global" : "Server"))
                .setDescription("Top challenge solvers");

            int rank = 1;
            for (const auto& [user_id, points] : leaderboard) {
                std::string username;
                try {
                    auto user = cringe.cluster.user_get_sync(user_id);
                    username = user.username;
                } catch (...) {
                    username = fmt::format("User {}", user_id);
                }

                embed.addField(
                    fmt::format("#{} {}", rank++, username),
                    fmt::format("Total Points: {}", points),
                    true
                );
            }

            event.edit_original_response(dpp::message(
                event.command.channel_id,
                embed.build()
            ));

        } else if (subcommand.name == "stats") {
            auto [total_points, challenges_attempted, challenges_completed] =
                cringe.challenge_store->getUserStats(event.command.usr.id);

            auto embed = CringeEmbed()
                .setTitle("📊 Your Challenge Statistics")
                .setDescription(fmt::format(
                    "Keep solving challenges to improve your stats!\n"
                    "Use `/challenge view` to see available challenges."
                ))
                .addField("Total Points", fmt::format("🏆 {}", total_points), true)
                .addField("Challenges Attempted", fmt::format("🎯 {}", challenges_attempted), true)
                .addField("Challenges Completed", fmt::format("✅ {}", challenges_completed), true);

            if (challenges_completed > 0) {
                float completion_rate = (float)challenges_completed / challenges_attempted * 100;
                embed.addField(
                    "Completion Rate",
                    fmt::format("📈 {:.1f}%", completion_rate),
                    true
                );
            }

            event.edit_original_response(dpp::message(
                event.command.channel_id,
                embed.build()
            ));
        }
    } catch (const std::exception& e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError(fmt::format("An error occurred: {}", e.what())).build()
        ));
    }
}

void handle_challenge_interaction(CringeBot& cringe, const dpp::interaction_create_t& event) {
	const auto component = std::get<dpp::component_interaction>(event.command.data);
	const auto& custom_id = component.custom_id;
	if (custom_id == "challenge_select") {
		auto challenge_id = std::stoull(component.values[0]);
        auto challenge_opt = cringe.challenge_store->getChallenge(challenge_id);
        
        if (!challenge_opt) {
            event.reply(dpp::message("Challenge not found!").set_flags(dpp::m_ephemeral));
            return;
        }

        auto embed = ChallengeEmbed::createChallengeDetailEmbed(
            *challenge_opt,
            event.command.usr.id,
            *cringe.challenge_store
        );

        dpp::message msg(event.command.channel_id, embed.build());
        msg.add_component(ChallengeEmbed::createChallengeDetailRow(*challenge_opt, 1));
        event.edit_response(msg);
    }
    else if (custom_id.starts_with("challenge_submit_")) {
        auto challenge_id = std::stoull(custom_id.substr(16));
        
        dpp::interaction_modal_response modal("challenge_answer", "Submit Answer");
        modal.add_component(
            dpp::component()
                .set_label("Your Answer")
                .set_id("answer")
                .set_type(dpp::cot_text)
                .set_placeholder("Enter your answer here")
                .set_min_length(1)
                .set_max_length(100)
                .set_required(true)
        );
        
        event.dialog(modal);
    }
    else if (custom_id == "challenge_answer") {
        auto challenge_id = std::stoull(custom_id.substr(16));
        // auto answer = std::get<std::string>(event.get_value("answer"));
        
        // int current_part = cringe.challenge_store->getCurrentPart(
        //     event.command.usr.id,
        //     challenge_id
        // );
    	int current_part = 1;
    	std::string answer = "1";
        
        bool success = cringe.challenge_store->submitAnswer(
            event.command.usr.id,
            challenge_id,
            current_part,
            answer
        );

        auto challenge_opt = cringe.challenge_store->getChallenge(challenge_id);
        if (!challenge_opt) {
            event.reply(dpp::message("Challenge not found!").set_flags(dpp::m_ephemeral));
            return;
        }

        auto embed = ChallengeEmbed::createChallengeDetailEmbed(
            *challenge_opt,
            event.command.usr.id,
            *cringe.challenge_store
        );

        dpp::message msg(event.command.channel_id, embed.build());
        msg.add_component(ChallengeEmbed::createChallengeDetailRow(*challenge_opt, current_part));

        if (success) {
            msg.add_embed(
                CringeEmbed::createSuccess(fmt::format(
                    "✅ Correct! You've earned {} points!",
                    challenge_opt->parts[current_part - 1].points
                )).build()
            );
        } else {
            msg.add_embed(
                CringeEmbed::createError(
                    "❌ Incorrect answer or you haven't completed previous parts."
                ).build()
            );
        }

        event.edit_response(msg);
    }
    else if (custom_id == "challenge_back") {
        auto challenges = cringe.challenge_store->getAllChallenges();
        auto embed = ChallengeEmbed::createChallengeListEmbed(challenges);
        auto components = ChallengeEmbed::createChallengeListRow(challenges);

        dpp::message msg(event.command.channel_id, embed.build());
        msg.add_component(components);
        event.edit_response(msg);
    }
}