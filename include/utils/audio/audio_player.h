// Jan 8
#pragma once
#include <dpp/dpp.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include "track.h"
#include "audio_processor.h"

namespace cringe {

enum class RepeatMode {
    NONE,
    ONE,
    ALL
};

class AudioPlayer {
private:
    dpp::cluster& bot;
    dpp::snowflake guild_id;
    dpp::discord_voice_client* voice_client;
    
    std::atomic<bool> is_playing{false};
    std::atomic<bool> is_paused{false};
    std::atomic<float> volume{1.0f};
    std::atomic<float> speed{1.0f};
    std::atomic<float> bass_boost{1.0f};
    
    RepeatMode repeat_mode{RepeatMode::NONE};
    std::shared_ptr<Track> current_track;
    std::unique_ptr<AudioProcessor> audio_processor;
    
    mutable std::mutex queue_mutex;
    std::deque<std::shared_ptr<Track>> queue;
    
    std::mutex playback_mutex;
    std::atomic<uint64_t> current_position{0};
    std::thread playback_thread;
    
    void stream_audio();
    void handle_playback_end();
    void update_position();

    std::string build_ffmpeg_command(const std::string& url) const {
        return fmt::format(
            "ffmpeg -reconnect 1 -reconnect_streamed 1 -reconnect_delay_max 5 "
            "-i \"{}\" -af \"volume={},asetrate=48000*{},aresample=48000,bass=g={}\" "
            "-f s16le -ar 48000 -ac 2 -loglevel error pipe:1",
            url,
            volume.load(),
            speed.load(),
            bass_boost.load()
        );
    }

public:
    AudioPlayer(dpp::cluster& bot, dpp::snowflake guild_id);
    ~AudioPlayer();
    
    // Connection management
    bool connect(dpp::snowflake channel_id);
    void disconnect();
    bool is_connected() const;
    
    // Playback control
    void play(std::shared_ptr<Track> track);
    void pause();
    void resume();
    void stop();
    void skip();
    
    // Queue management
    void add_to_queue(std::shared_ptr<Track> track);
    bool remove_from_queue(size_t index);
    void clear_queue();
    std::vector<std::shared_ptr<Track>> get_queue() const;
    void shuffle_queue();
    
    // Status and info
    std::shared_ptr<Track> get_current_track() const;
    uint64_t get_position() const { return current_position.load(); }
    bool is_playing_audio() const { return is_playing.load(); }
    bool is_paused_audio() const { return is_paused.load(); }
    
    // Audio effects
    void set_volume(float volume);
    void set_speed(float speed);
    void set_bass_boost(float boost);
    void set_repeat_mode(RepeatMode mode);
};

} // namespace cringe