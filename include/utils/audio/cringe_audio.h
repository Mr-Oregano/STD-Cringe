#ifndef CRINGE_CRINGE_AUDIO_H
#define CRINGE_CRINGE_AUDIO_H

#include <iostream>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <dpp/dpp.h>
#include "utils/audio/cringe_song.h"


enum class StreamStatus {
	Completed,
	Skipped,
	Error,
	Stopped
};

enum class LoopMode {
	NONE,
	SINGLE,
	QUEUE
};

struct AudioChunk {
    std::vector<std::byte> data;
    size_t size;
    dpp::voiceconn* voice;
    bool is_opus{false};          // Flag to indicate if data is OPUS encoded
    uint32_t timestamp{0};        // For tracking playback position
    uint32_t sample_count{0};     // Number of audio samples in this chunk
    std::string stream_id;
    static constexpr size_t MAX_QUEUE_SIZE = 100;
    static constexpr size_t OPUS_FRAME_SIZE = 960;  // Standard OPUS frame size
    static constexpr size_t PCM_FRAME_SIZE = 11520; // 120ms of 48KHz stereo PCM

    // Helper method to get the appropriate frame size
    size_t get_frame_size() const {
        return is_opus ? OPUS_FRAME_SIZE : PCM_FRAME_SIZE;
    }
};
//
// struct AudioChunk {
// 	static constexpr size_t PCM_FRAME_SIZE = 960 * 2 * sizeof(int16_t); // 960 samples * 2 channels * 2 bytes per sample
//
// 	std::vector<std::byte> data;
// 	size_t size;
// 	dpp::voiceconn* voice;
// 	bool is_opus;
// 	uint32_t timestamp;
// 	uint32_t duration;
// 	std::string stream_id;
//
// 	AudioChunk(std::vector<std::byte> d, size_t s, dpp::voiceconn* v,
// 			   bool opus, uint32_t ts, uint32_t dur, std::string sid)
// 		: data(std::move(d)), size(s), voice(v), is_opus(opus),
// 		  timestamp(ts), duration(dur), stream_id(std::move(sid)) {}
// };

class CringeAudioStreamer {
public:

    CringeAudioStreamer();
    ~CringeAudioStreamer();

    using StreamCallback = std::function<void(StreamStatus)>;
    using QueueCallback = std::function<std::optional<CringeSong>(dpp::snowflake)>;

	std::string build_ffmpeg_command(const std::string& url);

    // Prevent copying
    CringeAudioStreamer(const CringeAudioStreamer&) = delete;
    CringeAudioStreamer& operator=(const CringeAudioStreamer&) = delete;

	std::string get_time_string() const;
    
    // Core streaming functionality
    std::future<void> stream(dpp::voiceconn* voice, const CringeSong& song, StreamCallback on_complete = nullptr);
    void stop();
    void skip();
    
    // Playback control
    bool pause();
    bool resume();
    void set_volume(float volume);
    void set_speed(float speed);
    void set_bass_boost(float boost);
    void set_loop_mode(LoopMode mode);
    
    // State getters
    bool is_playing() const { return !should_stop_; }
    bool is_paused() const { return is_paused_; }
    float get_volume() const { return volume_; }
    float get_speed() const { return speed_; }
    float get_bass_boost() const { return bass_boost_; }
    LoopMode get_loop_mode() const { return loop_mode_; }
    std::mutex& getMutex() { return queue_mutex_; }

    // Set the queue callback
    void set_queue_callback(QueueCallback callback) {
        queue_callback_ = std::move(callback);
    }

    // Advanced playback control
    void seek(float position_seconds);
    void set_equalizer(const std::vector<float>& bands);
    void set_crossfade(float seconds);
    void set_normalize(bool enabled);
    
    // Advanced getters
    float get_position() const { return current_position_; }
    std::chrono::seconds get_duration() const;
    bool is_crossfade_enabled() const { return crossfade_duration_ > 0.0f; }
    
    // Queue management
    void shuffle_queue();
    void clear_history();
    size_t queue_size() const;

private:
    struct StreamContext {
        dpp::snowflake guild_id;
        std::shared_ptr<StreamCallback> callback;
    };

    void stream_audio(dpp::voiceconn* voice, const CringeSong& song, const std::string& cmd);
    void process_audio_queue();
    void clear_queue();

    // Audio state
    LoopMode loop_mode_{LoopMode::NONE};
    float current_position_{0.0f};
    std::atomic<bool> autoplay_enabled_{false};
    std::atomic<bool> is_paused_{false};
    std::atomic<float> volume_{1.0f};
    std::atomic<float> speed_{1.0f};
    std::atomic<float> bass_boost_{1.0f};

    // Opus encoding
    OpusEncoder* opus_encoder_{nullptr};
    static constexpr int OPUS_SAMPLE_RATE = 48000;
    static constexpr int OPUS_CHANNELS = 2;
    static constexpr int OPUS_FRAME_SIZE = 960;
    
    // Process management
    pid_t ytdlp_pid_{0};
    pid_t ffmpeg_pid_{0};

    // Thread management
    std::atomic<bool> should_stop_{false};
    std::atomic<bool> should_skip_{false};
    std::atomic<bool> running_{false};
    std::thread worker_thread_;
    
    // Queue and threading
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::string current_stream_id_;
    QueueCallback queue_callback_;
    std::queue<AudioChunk> audio_queue_;
    std::deque<CringeSong> history_queue_;

    std::atomic<float> crossfade_duration_{0.0f};
    std::atomic<bool> normalize_volume_{false};
    std::vector<float> equalizer_bands_{10, 1.0f}; // 10-band equalizer

    // Enhanced audio processing
    void apply_effects(int16_t* buffer, size_t size);
    void apply_equalizer(int16_t* buffer, size_t size);
    void apply_crossfade(int16_t* buffer, size_t size);
    void normalize_audio(int16_t* buffer, size_t size);
};

#endif // CRINGE_CRINGE_AUDIO_H