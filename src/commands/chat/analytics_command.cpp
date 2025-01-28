#include "commands/chat/analytics_command.h"
#include "utils/embed/cringe_embed.h"
#include <mutex>
#include <algorithm>
#include <future>

dpp::slashcommand analytics_declaration() {
    return dpp::slashcommand()
        .set_name("analytics")
        .set_description("View server engagement analytics")
        .add_option(
            dpp::command_option(dpp::co_sub_command, "top", "View top users")
            .add_option(
                dpp::command_option(dpp::co_integer, "count", "Number of users to show (max 25)", true)
            )
        )
        .add_option(
            dpp::command_option(dpp::co_sub_command, "overview", "Server overview")
        );
}

std::string repeat_str(const std::string& str, size_t n) {
    std::string result;
    result.reserve(str.size() * n);
    for (size_t i = 0; i < n; ++i) {
        result += str;
    }
    return result;
}

void analytics_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking();

    try {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();
        auto& subcommand = cmd_data.options[0];

        if (subcommand.name == "top") {
            // Get number of users to show (max 25)
            int64_t count = std::min(
                std::get<int64_t>(subcommand.options[0].value),
                static_cast<int64_t>(25)
            );

            // Get top posters from message store
            auto top_users = cringe.message_store->getTopPosters(
                event.command.guild_id,
                count
            );

            if (top_users.empty()) {
                event.edit_original_response(dpp::message(
                    event.command.channel_id,
                    CringeEmbed::createError("No message data found for this server!").build()
                ));
                return;
            }

            // Create embed
            auto embed = CringeEmbed()
                .setTitle("Cringe Analytics")
                .setDescription(fmt::format(
                    "Showing top {} most active users in this server\n\n"
                    "**Total Messages Analyzed:** {}\n",
                    count,
                    std::accumulate(top_users.begin(), top_users.end(), 0ULL,
                        [](size_t sum, const auto& pair) { return sum + pair.second; })
                ))
                .setColor(CringeColor::CringePrimary)
                .setThumbnail(CringeIcon::InfoIcon)
                .setTimestamp(time(nullptr));

            // Process each user
            std::string leaderboard;
            int rank = 1;
            
            // Calculate max messages for percentage bar
            size_t max_messages = top_users.empty() ? 1 : top_users[0].second;

            for (const auto& [user_id, message_count] : top_users) {
	            try {
	            	auto user = cringe.cluster.user_get_sync(user_id);

	            	// Calculate percentage of max messages (for visual bar)
	            	float percentage = static_cast<float>(message_count) / max_messages;
	            	int bar_segments = static_cast<int>(percentage * 10);

	            	// Create progress bar using our repeat function
	            	const int BAR_WIDTH = 15;
                    int filled_segments = static_cast<int>(percentage * BAR_WIDTH);
                    std::string progress_bar = fmt::format("`{}{}`",
                        repeat_str("█", filled_segments),
                        repeat_str("░", BAR_WIDTH - filled_segments)
                    );
                    
	            	// Format leaderboard entry
	            	leaderboard += fmt::format(
                        "{} **{}**\n{} `{:>6.1f}%` **{}** messages\n\n",
                        rank == 1 ? "👑" : fmt::format("#{}", rank),
                        user.username,
                        progress_bar,
                        percentage * 100,
                        message_count
                    );

	            	rank++;
	            } catch (...) {
	            	// ... error handling ...
	            }
            }

            embed.addField("Leaderboard", leaderboard, false)
                .setFooter(
                    fmt::format("Requested by {}", event.command.usr.global_name),
                    event.command.usr.get_avatar_url()
                );

            event.edit_original_response(dpp::message(
                event.command.channel_id,
                embed.build()
            ));
        } else if (subcommand.name == "overview") {
            // Get active channels
            auto active_channels = cringe.message_store->getActiveChannels(
                event.command.guild_id,
                5  // Top 5 channels
            );

            auto embed = CringeEmbed()
                .setTitle("📈 Server Analytics Overview")
                .setDescription("General server engagement statistics");

            // Add most active channels
            std::string channels_str;
            int channel_rank = 1;
            for (const auto& channel_id : active_channels) {
                try {
                    auto channel = cringe.cluster.channel_get_sync(channel_id);
                    channels_str += fmt::format("#{} {}\n", channel_rank++, channel.name);
                } catch (...) {
                    channels_str += fmt::format("#{} Unknown Channel\n", channel_rank++);
                }
            }

            if (!channels_str.empty()) {
                embed.addField("Most Active Channels", channels_str, false);
            }

            event.edit_original_response(dpp::message(
                event.command.channel_id,
                embed.build()
            ));
        }

    } catch (const std::exception& e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError(fmt::format("Error: {}", e.what())).build()
        ));
    }
}