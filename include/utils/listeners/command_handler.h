// #pragma once
// #include <dpp/dpp.h>
// #include "utils/audio/player.h"
// #include <unordered_map>
// #include <memory>
//
// namespace cringe {
//
// class CommandHandler {
// private:
//     dpp::cluster& bot;
//     std::unordered_map<dpp::snowflake, std::unique_ptr<Player>> guild_players;
//     mutable std::mutex players_mutex;
//
//     // Helper methods
//     Player& get_player(dpp::snowflake guild_id);
//     bool check_voice_state(const dpp::slashcommand_t& event);
//     void register_commands();
//     bool check_voice_permissions(const dpp::guild* guild,
//                             const dpp::guild_member* member,
//                             dpp::snowflake channel_id);
//
//     dpp::snowflake get_voice_channel(const dpp::guild* guild,
//                                     const dpp::user* user);
//
//
// public:
//     explicit CommandHandler(dpp::cluster& bot);
//     void handle_interaction(const dpp::slashcommand_t& event);
//
//     // Command handlers
//     void handle_play(const dpp::slashcommand_t& event);
//     void handle_pause(const dpp::slashcommand_t& event);
//     void handle_resume(const dpp::slashcommand_t& event);
//     void handle_stop(const dpp::slashcommand_t& event);
//     void handle_skip(const dpp::slashcommand_t& event);
//     void handle_queue(const dpp::slashcommand_t& event);
//     void handle_remove(const dpp::slashcommand_t& event);
//     void handle_nowplaying(const dpp::slashcommand_t& event);
//     void handle_repeat(const dpp::slashcommand_t& event);
//     void handle_shuffle(const dpp::slashcommand_t& event);
//     void handle_volume(const dpp::slashcommand_t& event);
//     void handle_playlist(const dpp::slashcommand_t& event);
//     void handle_search(const dpp::slashcommand_t& event);
//     void handle_effects(const dpp::slashcommand_t& event);
// };
//
// } // namespace cringe