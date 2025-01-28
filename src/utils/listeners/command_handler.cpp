// #include "utils/listeners/command_handler.h"
// #include <fmt/format.h>
// #include "commands/voice/music_player.h"
//
// namespace cringe {
//
// CommandHandler::CommandHandler(dpp::cluster& bot) : bot(bot) {
//     register_commands();
// }
//
// void CommandHandler::register_commands() {
//     std::vector<dpp::slashcommand> commands;
//
//     // Play command
//     dpp::slashcommand play_cmd("play", "Play a song or add it to queue", bot.me.id);
//     play_cmd.add_option(dpp::command_option(dpp::co_string, "query", "Song URL or search query", true));
//     commands.push_back(play_cmd);
//
//     // Basic playback controls
//     commands.push_back(dpp::slashcommand("pause", "Pause the current song", bot.me.id));
//     commands.push_back(dpp::slashcommand("resume", "Resume playback", bot.me.id));
//     commands.push_back(dpp::slashcommand("stop", "Stop playback and clear queue", bot.me.id));
//     commands.push_back(dpp::slashcommand("skip", "Skip the current song", bot.me.id));
//
//     // Queue management
//     dpp::slashcommand queue_cmd("queue", "Show the current queue", bot.me.id);
//     queue_cmd.add_option(dpp::command_option(dpp::co_integer, "page", "Queue page number", false));
//     commands.push_back(queue_cmd);
//
//     // ... Add other commands ...
//
//     bot.global_bulk_command_create(commands);
// }
//
// bool CommandHandler::check_voice_permissions(const dpp::guild* guild,
//                                           const dpp::guild_member* member,
//                                           dpp::snowflake channel_id) {
//     if (!guild || !member) return false;
//
//     // Get the voice channel
//     auto channel = dpp::find_channel(channel_id);
//     if (!channel) return false;
//
//     // Check if bot has required permissions
//     auto bot_member = dpp::find_guild_member(guild->id, bot.me.id);
//     if (!bot_member.get_user()) return false;
//
//     uint64_t required_perms = dpp::p_view_channel |
//                              dpp::p_connect |
//                              dpp::p_speak;
//
//     return true;
// }
//
// dpp::snowflake CommandHandler::get_voice_channel(const dpp::guild* guild,
//                                                const dpp::user* user) {
//     if (!guild || !user) return 0;
//
//     // Find the voice state for the user
//     for (const auto& state : guild->voice_members) {
//         if (state.first == user->id) {
//             return state.second.channel_id;
//         }
//     }
//     return 0;
// }
//
// void CommandHandler::handle_interaction(const dpp::slashcommand_t& event) {
//     std::string command_name = event.command.get_command_name();
//
//     try {
//         if (command_name == "play") handle_play(event);
//         else if (command_name == "pause") handle_pause(event);
//         else if (command_name == "resume") handle_resume(event);
//         else if (command_name == "stop") handle_stop(event);
//         else if (command_name == "skip") handle_skip(event);
//         else if (command_name == "queue") handle_queue(event);
//         else if (command_name == "remove") handle_remove(event);
//         else if (command_name == "np") handle_nowplaying(event);
//         else if (command_name == "repeat") handle_repeat(event);
//         else if (command_name == "shuffle") handle_shuffle(event);
//         else if (command_name == "volume") handle_volume(event);
//         else if (command_name == "playlist") handle_playlist(event);
//         else if (command_name == "search") handle_search(event);
//         else if (command_name == "effects") handle_effects(event);
//     } catch (const std::exception& e) {
//         event.reply(fmt::format("❌ Error: {}", e.what()));
//     }
// }
//
// void CommandHandler::handle_play(const dpp::slashcommand_t& event) {
//     auto guild = dpp::find_guild(event.command.guild_id);
//     if (!guild) {
//         event.reply("❌ Could not find guild!");
//         return;
//     }
//
//     // Get user's voice channel
//     dpp::snowflake voice_channel_id = get_voice_channel(
//         guild,
//         &event.command.get_issuing_user()
//     );
//
//     if (!voice_channel_id) {
//         event.reply("❌ You must be in a voice channel!");
//         return;
//     }
//
//     // Check permissions
//     auto member = dpp::find_guild_member(guild->id, event.command.usr.id);
//     if (!check_voice_permissions(guild, &member, voice_channel_id)) {
//         event.reply("❌ Missing required voice permissions!");
//         return;
//     }
//
//     std::string query = std::get<std::string>(event.get_parameter("query"));
//     event.thinking(true);
//
//     try {
//         auto& player = get_player(event.command.guild_id);
//
//         // Connect to voice channel if needed
//         if (!player.is_connected()) {
//             if (!player.connect(voice_channel_id)) {
//                 event.edit_response("❌ Failed to connect to voice channel!");
//                 return;
//             }
//         }
//
//         // Create and play track
//         auto track = TrackFactory::create_from_url(
//             query,
//             event.command.get_issuing_user().username,
//             event.command.usr.id
//         );
//
//         player.play(track);
//         event.edit_response(fmt::format("🎵 Added to queue: {}", track->get_info().title));
//
//     } catch (const std::exception& e) {
//         event.edit_response(fmt::format("❌ Error: {}", e.what()));
//     }
// }
//
// // ... (previous code remains the same) ...
//
// void CommandHandler::handle_pause(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     auto& player = get_player(event.command.guild_id);
//     player.pause();
//     event.reply("⏸️ Playback paused");
// }
//
// void CommandHandler::handle_resume(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     auto& player = get_player(event.command.guild_id);
//     player.resume();
//     event.reply("▶️ Playback resumed");
// }
//
// void CommandHandler::handle_stop(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     auto& player = get_player(event.command.guild_id);
//     player.stop();
//     player.clear_queue();
//     event.reply("⏹️ Playback stopped and queue cleared");
// }
//
// void CommandHandler::handle_skip(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     auto& player = get_player(event.command.guild_id);
//     player.skip();
//     event.reply("⏭️ Skipped current track");
// }
//
// void CommandHandler::handle_queue(const dpp::slashcommand_t& event) {
//     auto& player = get_player(event.command.guild_id);
//     auto queue = player.get_queue();
//
//     if (queue.empty()) {
//         event.reply("Queue is empty!");
//         return;
//     }
//
//     int64_t page = 1;
//     if (event.get_parameter("page").index() != 0) {
//         page = std::get<int64_t>(event.get_parameter("page"));
//     }
//
//     const size_t items_per_page = 10;
//     const size_t total_pages = (queue.size() + items_per_page - 1) / items_per_page;
//     page = std::clamp<int64_t>(page, 1, total_pages);
//
//     std::string response = "🎵 **Queue:**\n";
//     size_t start = (page - 1) * items_per_page;
//     size_t end = std::min(start + items_per_page, queue.size());
//
//     for (size_t i = start; i < end; i++) {
//         const auto& track = queue[i];
//         response += fmt::format("{}. {} [{}] (Requested by {})\n",
//             i + 1,
//             track->get_info().title,
//             track->get_info().duration,
//             track->get_info().requester_name);
//     }
//
//     response += fmt::format("\nPage {}/{}", page, total_pages);
//     event.reply(response);
// }
//
// void CommandHandler::handle_remove(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     auto index = std::get<int64_t>(event.get_parameter("position")) - 1;
//     auto& player = get_player(event.command.guild_id);
//
//     if (player.remove_from_queue(index)) {
//         event.reply(fmt::format("✅ Removed track at position {}", index + 1));
//     } else {
//         event.reply("❌ Invalid queue position!");
//     }
// }
//
// void CommandHandler::handle_nowplaying(const dpp::slashcommand_t& event) {
//     auto& player = get_player(event.command.guild_id);
//     auto current = player.get_current_track();
//
//     if (!current) {
//         event.reply("No track currently playing");
//         return;
//     }
//
//     const auto& info = current->get_info();
//     std::string response = fmt::format(
//         "🎵 **Now Playing:**\n{} [{}]\nRequested by: {}",
//         info.title,
//         info.duration,
//         info.requester_name
//     );
//
//     event.reply(response);
// }
//
// void CommandHandler::handle_repeat(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     std::string mode = std::get<std::string>(event.get_parameter("mode"));
//     auto& player = get_player(event.command.guild_id);
//
//     if (mode == "off") {
//         player.set_repeat_mode(RepeatMode::NONE);
//         event.reply("🔁 Repeat mode: Off");
//     } else if (mode == "one") {
//         player.set_repeat_mode(RepeatMode::ONE);
//         event.reply("🔂 Repeat mode: One");
//     } else if (mode == "all") {
//         player.set_repeat_mode(RepeatMode::ALL);
//         event.reply("🔁 Repeat mode: All");
//     }
// }
//
// void CommandHandler::handle_shuffle(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     auto& player = get_player(event.command.guild_id);
//     player.shuffle();
//     event.reply("🔀 Queue shuffled!");
// }
//
// void CommandHandler::handle_volume(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     float volume = std::get<double>(event.get_parameter("level")) / 100.0f;
//     auto& player = get_player(event.command.guild_id);
//     player.set_volume(volume);
//     event.reply(fmt::format("🔊 Volume set to {}%", static_cast<int>(volume * 100)));
// }
//
// void CommandHandler::handle_playlist(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     std::string url = std::get<std::string>(event.get_parameter("url"));
//     event.thinking(true);
//
//     try {
//         auto& player = get_player(event.command.guild_id);
//         auto tracks = TrackFactory::create_from_playlist(
//             url,
//             event.command.get_issuing_user().username,
//             event.command.usr.id
//         );
//
//         if (tracks.empty()) {
//             event.edit_response("❌ No tracks found in playlist!");
//             return;
//         }
//
//         for (const auto& track : tracks) {
//             player.play(track);
//         }
//
//         event.edit_response(fmt::format("✅ Added {} tracks from playlist!", tracks.size()));
//     } catch (const std::exception& e) {
//         event.edit_response(fmt::format("❌ Error: {}", e.what()));
//     }
// }
//
// void CommandHandler::handle_search(const dpp::slashcommand_t& event) {
//     std::string query = std::get<std::string>(event.get_parameter("query"));
//     event.thinking(true);
//
//     try {
//         std::string cmd = fmt::format(
//             "yt-dlp ytsearch5:\"{}\" --get-title --get-duration --get-url --format bestaudio",
//             query
//         );
//
//         std::string result = exec(cmd.c_str());
//         std::stringstream ss(result);
//         std::vector<std::string> titles, durations, urls;
//
//         std::string line;
//         int count = 0;
//         while (std::getline(ss, line) && count < 15) {
//             if (count % 3 == 0) titles.push_back(line);
//             else if (count % 3 == 1) durations.push_back(line);
//             else urls.push_back(line);
//             count++;
//         }
//
//         std::string response = "🔎 **Search Results:**\n";
//         for (size_t i = 0; i < titles.size(); i++) {
//             response += fmt::format("{}. {} [{}]\n",
//                 i + 1, titles[i], durations[i]);
//         }
//         response += "\nUse `/play` with the number to play a track!";
//
//         event.edit_response(response);
//     } catch (const std::exception& e) {
//         event.edit_response(fmt::format("❌ Error: {}", e.what()));
//     }
// }
//
// void CommandHandler::handle_effects(const dpp::slashcommand_t& event) {
//     if (!check_voice_state(event)) return;
//
//     auto& player = get_player(event.command.guild_id);
//     std::string effect = std::get<std::string>(event.get_parameter("effect"));
//     double value = std::get<double>(event.get_parameter("value"));
//
//     if (effect == "bass") {
//         player.set_bass_boost(value / 100.0f);
//         event.reply(fmt::format("🎛️ Bass boost set to {}%", value));
//     } else if (effect == "speed") {
//         player.set_speed(value / 100.0f);
//         event.reply(fmt::format("⚡ Speed set to {}%", value));
//     }
// }
//
// Player& CommandHandler::get_player(dpp::snowflake guild_id) {
//     std::lock_guard<std::mutex> lock(players_mutex);
//
//     auto it = guild_players.find(guild_id);
//     if (it == guild_players.end()) {
//         auto [inserted_it, success] = guild_players.emplace(
//             guild_id, std::make_unique<Player>(bot, guild_id));
//         return *inserted_it->second;
//     }
//     return *it->second;
// }
//
// bool CommandHandler::check_voice_state(const dpp::slashcommand_t& event) {
//     return true;
// }
//
// } // namespace cringe