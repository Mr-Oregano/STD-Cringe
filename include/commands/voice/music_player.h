#pragma once
#include <dpp/dpp.h>
#include <queue>
#include <mutex>
#include <string>
#include <vector>
#include <atomic>

enum class RepeatMode {
    NONE,
    ONE,
    ALL
};

struct Track {
    std::string url;
    std::string title;
    std::string duration;
    std::string requester_name;
    dpp::snowflake requester_id;
};

std::string exec(const char* cmd);

class MusicPlayer {
private:
    dpp::cluster& bot;
    std::queue<Track> queue;
    std::mutex queue_mutex;
    std::atomic<bool> is_playing;
    std::atomic<bool> is_paused;
    std::atomic<float> volume;
    std::atomic<float> bass_boost;
    std::atomic<float> speed;
    RepeatMode repeat_mode;
    Track current_track;
    dpp::snowflake guild_id;
    dpp::discord_voice_client* voice_client;

public:
	std::string get_ffmpeg_command(const std::string& url);
	explicit MusicPlayer(dpp::cluster& bot);
    
    // Connection status
    bool is_connected() const;

    // Playback control
    void handle_play_request(const std::string& query, const dpp::slashcommand_t& event);
    void start_playback(const Track& track, const dpp::slashcommand_t& event);
    bool pause();
    bool resume();
    bool skip();
    bool stop();

    // Queue management
    void clear_queue();
    void add_to_queue(const Track& track);
    bool remove_from_queue(size_t index);
    std::vector<Track> get_queue();
    Track get_current_track() const;

    // Playback settings
    void set_volume(float new_volume);
    void set_repeat_mode(RepeatMode mode);
    bool shuffle();

    // Search and playlist
    void handle_search(const std::string& query, const dpp::slashcommand_t& event);
    void add_playlist(const std::string& url, const dpp::slashcommand_t& event);

    // Audio effects
    void set_bass_boost(float level);
    void set_speed(float speed);
	void restart_playback_if_needed();
};