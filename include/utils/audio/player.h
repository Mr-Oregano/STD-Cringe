// #pragma once
// #include <dpp/dpp.h>
// #include "track.h"
// #include "audio_processing.h"
// #include <queue>
// #include <memory>
// #include <mutex>
// #include <atomic>
//
// namespace cringe {
//
// enum class RepeatMode {
//     NONE,
//     ONE,
//     ALL
// };
//
// class Player {
// private:
//     dpp::cluster& bot;
//     dpp::snowflake guild_id;
//     dpp::discord_voice_client* voice_client;
//     // std::unique_ptr<VoiceState> voice_state;
//
//     std::atomic<bool> is_playing;
//     std::atomic<bool> is_paused;
//     std::atomic<float> volume;
//     std::atomic<float> speed;
//     std::atomic<float> bass_boost;
//
//     RepeatMode repeat_mode;
//     std::shared_ptr<Track> current_track;
//     // QueueManager queue_manager;
//
//     std::mutex playback_mutex;
//     FILE* current_stream{nullptr};
//
//     void stream_audio();
//     void handle_playback_end();
//     void restart_playback_if_needed();
//
// public:
//     Player(dpp::cluster& bot, dpp::snowflake guild_id);
//     ~Player();
//
//     bool connect(dpp::snowflake channel_id);
//     void disconnect();
//     bool is_connected() const;
//
//     void play(std::shared_ptr<Track> track);
//     void pause();
//     void resume();
//     void stop();
//     void skip();
//
//     void set_volume(float volume);
//     void set_speed(float speed);
//     void set_bass_boost(float boost);
//     void set_repeat_mode(RepeatMode mode);
//
//     // Queue management
//     void add_to_queue(std::shared_ptr<Track> track);
//     bool remove_from_queue(size_t index);
//     void clear_queue();
//     std::vector<std::shared_ptr<Track>> get_queue() const;
//     std::shared_ptr<Track> get_current_track() const;
//     void shuffle();
//
//     // Voice state getters
//     dpp::snowflake get_channel_id() const { return voice_state->channel_id; }
//     dpp::snowflake get_guild_id() const { return guild_id; }
//     bool is_playing_audio() const { return is_playing.load(); }
//     bool is_paused_audio() const { return is_paused.load(); }
// };
//
// } // namespace cringe