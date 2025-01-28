// Jan 8
#include "utils/audio/audio_player.h"
#include <fmt/format.h>
#include <algorithm>
#include <random>
#include <chrono>

namespace cringe {

AudioPlayer::AudioPlayer(dpp::cluster& bot, dpp::snowflake guild_id)
    : bot(bot), guild_id(guild_id), voice_client(nullptr) {
    audio_processor = std::make_unique<AudioProcessor>();
}

AudioPlayer::~AudioPlayer() {
    stop();
    disconnect();
}

bool AudioPlayer::connect(dpp::snowflake channel_id) {
    if (voice_client && voice_client->is_ready()) {
        return true;
    }

    try {
        // Connect to voice channel
        bot.connect_voice(guild_id, channel_id, false, false);
        
        // Wait for connection to establish
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        voice_client = bot.get_voice(guild_id);
        return voice_client != nullptr;
    } catch (const std::exception& e) {
        fmt::print("Voice connection error: {}\n", e.what());
        return false;
    }
}

void AudioPlayer::disconnect() {
    std::lock_guard<std::mutex> lock(playback_mutex);
    stop();
    
    if (voice_client) {
        voice_client->disconnect_voice();
        voice_client = nullptr;
    }
}

void AudioPlayer::play(std::shared_ptr<Track> track) {
    if (!is_connected()) {
        return;
    }

    if (!voice_client || !voice_client->is_ready()) {
        return;
    }

    std::lock_guard<std::mutex> lock(playback_mutex);

    stop();

        current_track = track;
        is_playing = true;
        is_paused = false;
        current_position = 0;
        
        // Start playback in a new thread
        playback_thread = std::thread([this]() {
            try {
                stream_audio();
            } catch (const std::exception& e) {
                fmt::print("Playback error: {}\n", e.what());
                handle_playback_end();
            }
        });
        playback_thread.detach();
    } else {
        add_to_queue(track);
    }
}

void AudioPlayer::stream_audio() {
    std::lock_guard<std::mutex> lock(playback_mutex);
    
    if (!current_track || !voice_client || !voice_client->is_ready()) {
        return;
    }

    try {
        // Get stream URL from track
        std::string stream_url = current_track->get_stream_url();
        
        // Configure FFMPEG command with current audio settings
        // std::string ffmpeg_cmd = build_ffmpeg_command(stream_url);

        std::string filter_complex = fmt::format(
            "asetrate=48000*{},aresample=48000,bass=g={}",
            speed.load(),
            bass_boost.load()
        );
        
        std::string ffmpeg_cmd = fmt::format(
            "ffmpeg -i \"{}\" -af \"{}\" -f s16le -ar 48000 -ac 2 -loglevel quiet pipe:1",
            stream_url,
            filter_complex
        );

        // Start streaming through audio processor
        audio_processor->process_stream(ffmpeg_cmd, 
            [this](const uint8_t* data, size_t length) -> bool {
                if (!is_playing || is_paused || !voice_client || !voice_client->is_ready()) {
                    return false;
                }
                
                voice_client->send_audio_raw(
                    reinterpret_cast<const uint16_t*>(data),
                    length / sizeof(uint16_t)
                );
                
                current_position += length / (48000.0 * 2 * 2); // Update position in seconds
                return true;
            }
        );

        handle_playback_end();

    } catch (const std::exception& e) {
        fmt::print("Streaming error: {}\n", e.what());
        throw;
    }
}

void AudioPlayer::handle_playback_end() {

    std::lock_guard<std::mutex> lock(playback_mutex);
    
    is_playing = false;
    current_position = 0;

    if (voice_client) {
        voice_client->stop_audio();
    }

    // Handle repeat modes
    if (repeat_mode == RepeatMode::ONE) {
        play(current_track);
        return;
    }

    // Get next track from queue
    std::shared_ptr<Track> next_track = nullptr;
    {
        std::lock_guard<std::mutex> queue_lock(queue_mutex);
        if (!queue.empty()) {
            next_track = queue.front();
            queue.pop_front();
        }
        
        if (repeat_mode == RepeatMode::ALL && !next_track && current_track) {
            queue.push_back(current_track);
            next_track = queue.front();
            queue.pop_front();
        }
    }

    current_track = nullptr;

    if (next_track) {
        play(next_track);
    }
}

void AudioPlayer::pause() {
    if (is_playing && !is_paused) {
        is_paused = true;
        if (voice_client) {
            voice_client->pause_audio();
        }
    }
}

void AudioPlayer::resume() {
    if (is_playing && is_paused) {
        is_paused = false;
        if (voice_client) {
            voice_client->resume_audio();
        }
    }
}

void AudioPlayer::stop() {
    std::lock_guard<std::mutex> lock(playback_mutex);
    
    is_playing = false;
    is_paused = false;
    current_position = 0;
    
    if (voice_client) {
        voice_client->stop_audio();
    }
    
    if (playback_thread.joinable()) {
        playback_thread.join();
    }
    
    current_track.reset();
}

// Queue management methods
void AudioPlayer::add_to_queue(std::shared_ptr<Track> track) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    queue.push_back(track);
}

bool AudioPlayer::remove_from_queue(size_t index) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (index >= queue.size()) return false;
    
    queue.erase(queue.begin() + index);
    return true;
}

void AudioPlayer::clear_queue() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    queue.clear();
}

std::vector<std::shared_ptr<Track>> AudioPlayer::get_queue() const {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return std::vector<std::shared_ptr<Track>>(queue.begin(), queue.end());
}

void AudioPlayer::shuffle_queue() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(queue.begin(), queue.end(), g);
}

// Audio effect methods
void AudioPlayer::set_volume(float new_volume) {
    volume.store(std::clamp(new_volume, 0.0f, 2.0f));
}

void AudioPlayer::set_speed(float new_speed) {
    speed.store(std::clamp(new_speed, 0.5f, 2.0f));
}

void AudioPlayer::set_bass_boost(float new_boost) {
    bass_boost.store(std::clamp(new_boost, 0.0f, 2.0f));
}

void AudioPlayer::set_repeat_mode(RepeatMode mode) {
    repeat_mode = mode;
}
