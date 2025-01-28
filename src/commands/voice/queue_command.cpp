#include "commands/voice/queue_command.h"
#include <fmt/format.h>
#include "utils/embed/cringe_embed.h"

#include "utils/bot/cringe_bot.h"

dpp::slashcommand queue_declaration() {
    return dpp::slashcommand()
        .set_name("queue")
        .set_description("Manage the music queue")
        .add_option(dpp::command_option(dpp::co_sub_command, "show", "Show current queue"))
        .add_option(dpp::command_option(dpp::co_sub_command, "remove", "Remove a song from queue")
            .add_option(dpp::command_option(dpp::co_integer, "position", "Position in queue", true)))
        .add_option(dpp::command_option(dpp::co_sub_command, "clear", "Clear the queue"));
}

void queue_command(CringeBot& cringe, const dpp::slashcommand_t& event) {
    event.thinking(true);
    
    try {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();
        auto& subcommand = cmd_data.options[0];
        
        if (subcommand.name == "show") {
            auto songs = cringe.queue_store->getQueue(event.command.guild_id);
            
            if (songs.empty()) {
                event.edit_original_response(dpp::message(
                    event.command.channel_id,
                    CringeEmbed::createInfo("Queue is empty").build()
                ));
                return;
            }
            
            std::string queue_list;
            int total_duration = 0;
            
            for (size_t i = 0; i < songs.size(); i++) {
                const auto& song = songs[i];
                if (song.duration) {
                    total_duration += static_cast<int>(song.duration->count()) / 60;
                }
                queue_list += fmt::format("\n**{}. {}**\nDuration: {} minutes\n",
                    i + 1, song.title,
                    song.duration ? (song.duration->count() / 60) : 0);
            }
            
            auto embed = CringeEmbed::createCommand("queue", "Current music queue")
                .setThumbnail(CringeIcon::MusicIcon)
                .addField("Queue Info",
                    fmt::format("\n**Total Songs**: {}\n**Queue Duration**: {} minutes\n",
                        songs.size(), total_duration))
                .addField("Tracks", queue_list);
                
            event.edit_original_response(dpp::message(
                event.command.channel_id, embed.build()
            ));
        }
        else if (subcommand.name == "remove") {
            size_t position = std::get<int64_t>(subcommand.options[0].value) - 1;
            cringe.queue_store->removeSong(event.command.guild_id, position);
            
            event.edit_original_response(dpp::message(
                event.command.channel_id,
                CringeEmbed::createSuccess(
                    fmt::format("Removed song at position {}", position + 1)
                ).build()
            ));
        }
        else if (subcommand.name == "clear") {
            cringe.queue_store->clearQueue(event.command.guild_id);
            
            event.edit_original_response(dpp::message(
                event.command.channel_id,
                CringeEmbed::createSuccess("Queue cleared").build()
            ));
        }
    }
    catch (const std::exception& e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError(fmt::format("Error: {}", e.what())).build()
        ));
    }
}