#include "commands/voice/music_player.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fmt/format.h>
#include <memory>
#include <random>
#include <stdexcept>
#include <thread>
#include "utils/audio/player.h"

// Helper function to execute shell commands and get output
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

std::string MusicPlayer::get_ffmpeg_command(const std::string& url) {
    float current_speed = speed.load();
    float current_bass = bass_boost.load();
    
    return fmt::format(
        "ffmpeg -i \"{}\" -f s16le -ar 48000 -ac 2 -filter:a \"asetrate=44100*{},aresample=44100,bass=g={}\" -loglevel quiet pipe:1",
        url,
        current_speed,
        current_bass
    );
}

MusicPlayer::MusicPlayer(dpp::cluster& bot) 
    : bot(bot),
      is_playing(false),
      is_paused(false),
      volume(1.0f),
      repeat_mode(RepeatMode::NONE),
      voice_client(nullptr),
      bass_boost(1.0f),
      speed(1.0f) {
}

bool MusicPlayer::is_connected() const {
    return voice_client != nullptr;
}

void MusicPlayer::handle_play_request(const std::string& query, const dpp::slashcommand_t& event) {
    try {
        // Check if it's a URL or search query
        bool is_url = query.find("http") != std::string::npos;
        std::string cmd;
        
        if (is_url) {
            cmd = fmt::format("yt-dlp --get-title --get-duration --get-url \"{}\" -f bestaudio", query);
        } else {
            cmd = fmt::format("yt-dlp ytsearch1:\"{}\" --get-title --get-duration --get-url -f bestaudio", query);
        }

        std::string result = exec(cmd.c_str());
        std::stringstream ss(result);
        std::string title, duration, url;
        
        std::getline(ss, title);
        std::getline(ss, duration);
        std::getline(ss, url);

        if (url.empty()) {
            event.edit_response("❌ Could not find track!");
            return;
        }

        Track track{
            url,
            title,
            duration,
            event.command.get_issuing_user().username,
            event.command.usr.id
        };

        if (!is_playing) {
            current_track = track;
            start_playback(track, event);
        } else {
            add_to_queue(track);
            event.edit_response(fmt::format("🎵 Added to queue: {}", track.title));
        }
    } catch (const std::exception& e) {
        event.edit_response("❌ Error processing track: " + std::string(e.what()));
    }
}

void MusicPlayer::start_playback(const Track& track, const dpp::slashcommand_t& event) {
    if (!voice_client) {
        throw std::runtime_error("Not connected to voice channel!");
    }

    is_playing = true;
    is_paused = false;

    // Start playback in a separate thread
    std::thread([this, track, event]() {
        try {
            // Create ffmpeg command with filters
            std::string filter_complex = fmt::format(
                "asetrate=44100*{},aresample=44100,bass=g={}",
                speed.load(),
                bass_boost.load()
            );

            std::string cmd = fmt::format(
                "ffmpeg -i \"{}\" -af \"{}\" -f s16le -ar 48000 -ac 2 -loglevel quiet -",
                track.url,
                filter_complex
            );

            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) {
                event.edit_response("❌ Failed to start playback!");
                is_playing = false;
                return;
            }

            event.edit_response(fmt::format("🎵 Now playing: {}", track.title));

            // Read and send audio data
            std::vector<uint8_t> buffer(16384); // 16KB buffer
            while (is_playing && !is_paused) {
                size_t bytes_read = fread(buffer.data(), 1, buffer.size(), pipe);
                if (bytes_read == 0) break;

                // Apply volume
                if (volume != 1.0f) {
                    for (size_t i = 0; i < bytes_read; i += 2) {
                        int16_t& sample = *reinterpret_cast<int16_t*>(&buffer[i]);
                        sample = static_cast<int16_t>(sample * volume);
                    }
                }

                voice_client->send_audio_raw((uint16_t*)buffer.data(), bytes_read / 2);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }

            pclose(pipe);

            // Handle what to do next if we weren't stopped
            if (is_playing) {
                if (repeat_mode == RepeatMode::ONE) {
                    start_playback(track, event);
                } else {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    if (!queue.empty() || repeat_mode == RepeatMode::ALL) {
                        if (queue.empty() && repeat_mode == RepeatMode::ALL) {
                            queue.push(track);
                        }
                        Track next_track = queue.front();
                        queue.pop();
                        start_playback(next_track, event);
                    } else {
                        is_playing = false;
                        current_track = Track{};
                    }
                }
            }
        } catch (const std::exception& e) {
            event.edit_response("❌ Error during playback: " + std::string(e.what()));
            is_playing = false;
        }
    }).detach();
}

bool MusicPlayer::pause() {
    if (!is_playing || is_paused) return false;
    is_paused = true;
    if (voice_client) {
        voice_client->pause_audio(true);
    }
    return true;
}

bool MusicPlayer::resume() {
    if (!is_playing || !is_paused) return false;
    is_paused = false;
    if (voice_client) {
        voice_client->pause_audio(false);
    }
    return true;
}

bool MusicPlayer::skip() {
    if (!is_playing) return false;
    is_playing = false;
    return true;
}

bool MusicPlayer::stop() {
    if (!is_playing) return false;
    is_playing = false;
    clear_queue();
    if (voice_client) {
        voice_client->stop_audio();
        voice_client = nullptr;
    }
    return true;
}

void MusicPlayer::clear_queue() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::queue<Track>().swap(queue);
}

void MusicPlayer::add_to_queue(const Track& track) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    queue.push(track);
}

bool MusicPlayer::remove_from_queue(size_t index) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (index >= queue.size()) return false;

    std::queue<Track> new_queue;
    size_t current = 0;
    
    while (!queue.empty()) {
        if (current != index) {
            new_queue.push(queue.front());
        }
        queue.pop();
        current++;
    }
    
    queue = std::move(new_queue);
    return true;
}

std::vector<Track> MusicPlayer::get_queue() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::vector<Track> queue_list;
    auto temp_queue = queue;
    
    while (!temp_queue.empty()) {
        queue_list.push_back(temp_queue.front());
        temp_queue.pop();
    }
    
    return queue_list;
}

Track MusicPlayer::get_current_track() const {
    return current_track;
}

void MusicPlayer::set_volume(float new_volume) {
    volume.store(std::clamp(new_volume, 0.0f, 2.0f));
}

void MusicPlayer::set_repeat_mode(RepeatMode mode) {
    repeat_mode = mode;
}

bool MusicPlayer::shuffle() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (queue.empty()) return false;

    std::vector<Track> tracks;
    while (!queue.empty()) {
        tracks.push_back(queue.front());
        queue.pop();
    }
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(tracks.begin(), tracks.end(), g);
    
    for (const auto& track : tracks) {
        queue.push(track);
    }
    
    return true;
}

void MusicPlayer::handle_search(const std::string& query, const dpp::slashcommand_t& event) {
    try {
        std::string cmd = fmt::format(
            "yt-dlp ytsearch5:\"{}\" --get-title --get-duration --get-url -f bestaudio",
            query
        );
        
        std::string result = exec(cmd.c_str());
        std::stringstream ss(result);
        std::vector<Track> results;
        
        std::string title, duration, url;
        while (std::getline(ss, title) && std::getline(ss, duration) && std::getline(ss, url)) {
            results.push_back(Track{url, title, duration, "", 0});
        }

        if (results.empty()) {
            event.edit_response("❌ No results found!");
            return;
        }

        std::string response = "🔎 **Search Results:**\n";
        for (size_t i = 0; i < results.size(); i++) {
            response += fmt::format("{}. {} [{}]\n", 
                i + 1, results[i].title, results[i].duration);
        }
        response += "\nUse `/play` with the number or title to play a track!";
        
        event.edit_response(response);
    } catch (const std::exception& e) {
        event.edit_response("❌ Error during search: " + std::string(e.what()));
    }
}

void MusicPlayer::add_playlist(const std::string& url, const dpp::slashcommand_t& event) {
    try {
        std::string cmd = fmt::format(
            "yt-dlp --flat-playlist --get-title --get-duration --get-url \"{}\" -f bestaudio",
            url
        );
        
        std::string result = exec(cmd.c_str());
        std::stringstream ss(result);
        int tracks_added = 0;
        
        std::string title, duration, track_url;
        while (std::getline(ss, title) && std::getline(ss, duration) && std::getline(ss, track_url)) {
            Track track{
                track_url,
                title,
                duration,
                event.command.get_issuing_user().username,
                event.command.usr.id
            };

            if (!is_playing) {
                current_track = track;
                start_playback(track, event);
            } else {
                add_to_queue(track);
            }
            tracks_added++;
        }

        event.edit_response(fmt::format("✅ Added {} tracks from playlist!", tracks_added));
    } catch (const std::exception& e) {
        event.edit_response("❌ Error adding playlist: " + std::string(e.what()));
    }
}

void MusicPlayer::set_bass_boost(float new_bass) {
    bass_boost.store(std::clamp(new_bass, 0.0f, 2.0f));
    restart_playback_if_needed();
}

void MusicPlayer::set_speed(float new_speed) {
    speed.store(std::clamp(new_speed, 0.5f, 2.0f));
    restart_playback_if_needed();
}

void MusicPlayer::restart_playback_if_needed() {
    if (is_playing.load() && !is_paused.load()) {
        auto current = current_track;
        stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!current.url.empty()) {
            add_to_queue(current);
        }
    }
}