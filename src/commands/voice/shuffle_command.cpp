#include "commands/voice/shuffle_command.h"
#include <algorithm>
#include <random>
#include <vector>
#include <unordered_map>
#include "utils/embed/cringe_embed.h"

dpp::slashcommand shuffle_declaration() {
    return dpp::slashcommand()
        .set_name("shuffle")
        .set_description("Randomly shuffle users between voice channels")
        .add_option(dpp::command_option(dpp::co_integer, "duration", 
            "Duration in seconds to shuffle (max 30)", true));
}

void shuffle_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(true);
    
    int duration = std::get<int64_t>(event.get_parameter("duration"));
    if (duration > 30) duration = 30;
    
    auto guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        event.edit_original_response(dpp::message(event.command.channel_id, "Failed to find guild!"));
        return;
    }

    // Get all voice channels
    std::vector<dpp::snowflake> voice_channel_ids;
    for (const auto& channel : guild->channels) {
        if (auto c = dpp::find_channel(channel)) {
            if (c->is_voice_channel()) {
                voice_channel_ids.push_back(c->id);
            }
        }
    }

    if (voice_channel_ids.size() < 2) {
        event.edit_original_response(dpp::message(event.command.channel_id, 
            "Need at least 2 voice channels to shuffle!"));
        return;
    }

    // Get users in voice channels
    std::vector<std::pair<dpp::snowflake, dpp::snowflake>> user_channels;
    for (const auto& [user_id, voice_state] : guild->voice_members) {
        // Only include users who are in a voice channel
        if (voice_state.channel_id != 0) {
            user_channels.push_back({user_id, voice_state.channel_id});
        }
    }

    if (user_channels.empty()) {
        event.edit_original_response(dpp::message(event.command.channel_id, 
            "No users in voice channels to shuffle!"));
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    // Start shuffling thread
    std::thread([guild_id = guild->id, user_channels, voice_channel_ids, duration, 
                 channel_id = event.command.channel_id, cluster = &cringe.cluster]() {
        auto start_time = std::chrono::steady_clock::now();
        std::mt19937 thread_gen(std::random_device{}());
        
        while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count() < duration) {
                
            // Shuffle users
            for (const auto& [user_id, _] : user_channels) {
                std::uniform_int_distribution<size_t> dist(0, voice_channel_ids.size() - 1);
                auto new_channel_id = voice_channel_ids[dist(thread_gen)];
                
                try {
                    // Correct parameter order: channel_id, guild_id, user_id
                    cluster->guild_member_move(new_channel_id, guild_id, user_id);
                } catch (...) {
                    // Ignore errors and continue with next user
                }
                
                // Add delay between moves to avoid rate limiting
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
            
            // Add delay between shuffle rounds
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Return users to original channels
        for (const auto& [user_id, original_channel_id] : user_channels) {
            try {
                // Correct parameter order: channel_id, guild_id, user_id
                cluster->guild_member_move(original_channel_id, guild_id, user_id);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } catch (...) {
                // Ignore errors and continue
            }
        }

        // Send completion message
//        cluster->message_create(dpp::message(channel_id,
//            "Shuffle complete! Everyone has been returned to their original channels 😈"));
    }).detach();

    event.edit_original_response(dpp::message(event.command.channel_id, 
        "Let the chaos begin! 🎲"));
}