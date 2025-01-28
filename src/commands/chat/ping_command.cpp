#include "commands/chat/ping_command.h"
#include "utils/embed/cringe_embed.h"
#include <random>
#include <thread>

dpp::slashcommand ping_declaration() {
    return dpp::slashcommand()
        .set_name("ping")
        .set_description("Ping a user multiple times")
        .add_option(
            dpp::command_option(dpp::co_sub_command, "persistent", "Send multiple persistent pings")
                .add_option(dpp::command_option(dpp::co_user, "target", "User to ping", true))
                .add_option(dpp::command_option(dpp::co_integer, "count", "Number of pings (max 50)", true))
                .add_option(dpp::command_option(dpp::co_boolean, "scatter", "Scatter pings across channels", false))
        )
        .add_option(
            dpp::command_option(dpp::co_sub_command, "ghost", "Send multiple ghost pings")
                .add_option(dpp::command_option(dpp::co_user, "target", "User to ping", true))
                .add_option(dpp::command_option(dpp::co_integer, "count", "Number of pings (max 50)", true))
                .add_option(dpp::command_option(dpp::co_boolean, "scatter", "Scatter pings across channels", false))
        );
}

void ping_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(true);

    try {
        const auto& cmd_data = event.command.get_command_interaction();
        const auto& subcommand = cmd_data.options[0];
        
        auto target_user = std::get<dpp::snowflake>(subcommand.options[0].value);
        auto count = std::min(std::get<int64_t>(subcommand.options[1].value), static_cast<int64_t>(50));
        bool scatter = subcommand.options.size() > 2 ? 
            std::get<bool>(subcommand.options[2].value) : false;

    	// Get available channels if scatter is enabled
    	std::vector<dpp::snowflake> available_channels;
    	if (scatter) {
    		dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    		if (guild) {
    			for (const auto& channel : guild->channels) {
    				if (auto channel_obj = dpp::find_channel(channel)) {
    					if (channel_obj->is_text_channel()) {
    						available_channels.push_back(channel);
    					}
    				}
    			}
    		}
    	}

        if (scatter && available_channels.empty()) {
            event.edit_original_response(dpp::message().add_embed(
                CringeEmbed::createError("No available text channels found!")
                    .setTitle("Failed to Execute Ping Command")
                    .build()
            ));
            return;
        }

        // Create RNG for channel selection
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dis(0, available_channels.size() - 1);

        // Create success embed
        auto success_embed = CringeEmbed::createSuccess(
            fmt::format("Sending {} {} ping{} to <@{}>{}",
                count,
                subcommand.name,
                count > 1 ? "s" : "",
                target_user,
                scatter ? " scattered across channels" : ""
            )
        ).build();

        event.edit_original_response(dpp::message().add_embed(success_embed));

        // Create shared pointers for data needed in thread
        auto thread_cringe = &cringe;
        auto thread_event = std::make_shared<dpp::slashcommand_t>(event);
        auto thread_available_channels = std::make_shared<std::vector<dpp::snowflake>>(available_channels);

        // Start ping thread
        std::thread([thread_cringe, thread_event, target_user, count, scatter, 
                    thread_available_channels, subcommand]() {
            try {
                std::random_device thread_rd;
                std::mt19937 thread_gen(thread_rd());
                std::uniform_int_distribution<size_t> thread_dis(0, 
                    scatter ? thread_available_channels->size() - 1 : 0);

                for (int i = 0; i < count; i++) {
                    dpp::snowflake channel_id = scatter ? 
                        (*thread_available_channels)[thread_dis(thread_gen)] : 
                        thread_event->command.channel_id;

                    dpp::message msg(channel_id, fmt::format("<@{}> ping #{}", target_user, i + 1));
                    
                    if (subcommand.name == "ghost") {
                        // For ghost pings, send and immediately delete
                        auto sent_msg = thread_cringe->cluster.message_create_sync(msg);
                        thread_cringe->cluster.message_delete(sent_msg.id, channel_id);
                    } else {
                        // For persistent pings, just send
                        thread_cringe->cluster.message_create(msg);
                    }

                    // Add small delay between pings
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            } catch (const std::exception& e) {
                thread_cringe->logger.log(LogLevel::ERROR, 
                    fmt::format("Error in ping thread: {}", e.what()));
            }
        }).detach();

    } catch (const std::exception& e) {
        event.edit_original_response(dpp::message().add_embed(
            CringeEmbed::createError(fmt::format("Error: {}", e.what()))
                .setTitle("Failed to Execute Ping Command")
                .build()
        ));
    }
}