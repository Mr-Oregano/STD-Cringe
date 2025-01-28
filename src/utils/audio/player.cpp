// #include "utils/audio/player.h"
//
// #include <algorithm>
// #include <fmt/format.h>
// #include <random>
//
// namespace cringe {
//
// Player::Player(dpp::cluster& bot, dpp::snowflake guild_id)
//     : bot(bot), guild_id(guild_id) {}
//
// Player::~Player() {
//     stop();
//     disconnect();
// }
//
// bool Player::connect(dpp::snowflake channel_id) {
//     if (voice_client && voice_client->is_ready()) {
//         return true;
//     }
//
//     try {
//         // Store voice state
//         voice_state->channel_id = channel_id;
//         voice_state->guild_id = guild_id;
//
//         // Connect using Musicat's approach
//         bot.connect_voice(guild_id, channel_id, false, false);
//         std::this_thread::sleep_for(std::chrono::milliseconds(500));
//
//         voice_client = bot.get_voice(guild_id);
//         return voice_client != nullptr;
//     } catch (const std::exception& e) {
//         fmt::print("Voice connection error: {}\n", e.what());
//         return false;
//     }
// }
//
// void Player::disconnect() {
//     stop();
//     if (voice_client) {
//         voice_client->disconnect_voice();
//         voice_client = nullptr;
//     }
// }
//
// bool Player::is_connected() const {
//     return voice_client && voice_client->is_ready();
// }
//
// void Player::play(std::shared_ptr<Track> track) {
//     if (!is_connected()) {
//         return;
//     }
//
//     if (!is_playing) {
//         start_playback(track);
//     } else {
//         add_to_queue(track);
//     }
// }
//
// void Player::start_playback(std::shared_ptr<Track> track) {
//     std::lock_guard<std::mutex> lock(playback_mutex);
//
//     // Stop current playback if any
//     stop();
//
//     current_track = track;
//     is_playing = true;
//     is_paused = false;
//
//     // Start playback in a new thread
//     playback_thread = std::thread([this]() {
//         try {
//             stream_audio();
//         } catch (const std::exception&) {
//             handle_playback_end();
//         }
//     });
//     playback_thread.detach();
// }
//
// void Player::stream_audio() {
//     if (!current_track || !voice_client) return;
//
//     // Get stream URL
//     std::string stream_url = current_track->get_stream_url();
//
//     // Create ffmpeg command
//     std::string cmd = fmt::format(
//         "ffmpeg -i \"{}\" -f s16le -ar 48000 -ac 2 -loglevel quiet -",
//         stream_url
//     );
//
//     current_stream = popen(cmd.c_str(), "r");
//     if (!current_stream) {
//         throw std::runtime_error("Failed to start ffmpeg process");
//     }
//
//     // Read and send audio data
//     std::vector<int16_t> buffer(16384); // 32KB buffer (16384 * sizeof(int16_t))
//     while (is_playing && !is_paused) {
//         size_t bytes_read = fread(buffer.data(), sizeof(int16_t), buffer.size(), current_stream);
//         if (bytes_read == 0) break;
//
//         // Process audio
//         audio_processor.process_chunk(buffer.data(), bytes_read);
//
//         // Send to Discord
//         if (voice_client && voice_client->is_ready()) {
//             voice_client->send_audio_raw(buffer.data(), bytes_read);
//         }
//
//         // Control playback speed
//         std::this_thread::sleep_for(std::chrono::milliseconds(20));
//     }
//
//     // Clean up
//     if (current_stream) {
//         pclose(current_stream);
//         current_stream = nullptr;
//     }
//
//     // Handle playback end
//     if (is_playing) {
//         handle_playback_end();
//     }
// }
//
// void Player::handle_playback_end() {
//     std::lock_guard<std::mutex> lock(playback_mutex);
//
//     if (repeat_mode == RepeatMode::ONE) {
//         // Restart the same track
//         start_playback(current_track);
//         return;
//     }
//
//     // Get next track from queue
//     std::shared_ptr<Track> next_track = nullptr;
//     {
//         std::lock_guard<std::mutex> queue_lock(queue_mutex);
//         if (!queue.empty()) {
//             next_track = queue.front();
//             queue.pop();
//         } else if (repeat_mode == RepeatMode::ALL && current_track) {
//             next_track = current_track;
//         }
//     }
//
//     if (next_track) {
//         start_playback(next_track);
//     } else {
//         is_playing = false;
//         current_track = nullptr;
//     }
// }
//
// void Player::pause() {
//     if (is_playing && !is_paused) {
//         is_paused = true;
//         if (voice_client) {
//             voice_client->pause_audio(true);
//         }
//     }
// }
//
// void Player::resume() {
//     if (is_playing && is_paused) {
//         is_paused = false;
//         if (voice_client) {
//             voice_client->pause_audio(false);
//         }
//     }
// }
//
// void Player::stop() {
//     is_playing = false;
//     is_paused = false;
//
//     std::lock_guard<std::mutex> lock(playback_mutex);
//     if (current_stream) {
//         pclose(current_stream);
//         current_stream = nullptr;
//     }
//
//     if (voice_client) {
//         voice_client->stop_audio();
//     }
//
//     current_track = nullptr;
// }
//
// void Player::skip() {
//     stop();
//     handle_playback_end();
// }
//
// void Player::add_to_queue(std::shared_ptr<Track> track) {
//     std::lock_guard<std::mutex> lock(queue_mutex);
//     queue.push(track);
// }
//
// bool Player::remove_from_queue(size_t index) {
//     std::lock_guard<std::mutex> lock(queue_mutex);
//     if (index >= queue.size()) return false;
//
//     std::queue<std::shared_ptr<Track>> new_queue;
//     size_t current = 0;
//
//     while (!queue.empty()) {
//         if (current != index) {
//             new_queue.push(queue.front());
//         }
//         queue.pop();
//         current++;
//     }
//
//     queue = std::move(new_queue);
//     return true;
// }
//
// void Player::clear_queue() {
//     std::lock_guard<std::mutex> lock(queue_mutex);
//     std::queue<std::shared_ptr<Track>>().swap(queue);
// }
//
// std::vector<std::shared_ptr<Track>> Player::get_queue() const {
//     std::lock_guard<std::mutex> lock(queue_mutex);
//     std::vector<std::shared_ptr<Track>> queue_list;
//     auto temp_queue = queue;
//
//     while (!temp_queue.empty()) {
//         queue_list.push_back(temp_queue.front());
//         temp_queue.pop();
//     }
//
//     return queue_list;
// }
//
// std::shared_ptr<Track> Player::get_current_track() const {
//     return current_track;
// }
//
// void Player::set_volume(float volume) {
//     audio_processor.set_volume(volume);
// }
//
// void Player::set_speed(float speed) {
//     audio_processor.set_speed(speed);
// }
//
// void Player::set_bass_boost(float boost) {
//     audio_processor.set_bass_boost(boost);
// }
//
// void Player::set_repeat_mode(RepeatMode mode) {
//     repeat_mode = mode;
// }
//
// void Player::shuffle() {
//     std::lock_guard<std::mutex> lock(queue_mutex);
//     std::vector<std::shared_ptr<Track>> tracks;
//
//     while (!queue.empty()) {
//         tracks.push_back(queue.front());
//         queue.pop();
//     }
//
//     std::random_device rd;
//     std::mt19937 g(rd());
//     std::shuffle(tracks.begin(), tracks.end(), g);
//
//     for (const auto& track : tracks) {
//         queue.push(track);
//     }
// }
//
// } // namespace cringe