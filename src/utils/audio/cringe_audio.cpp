#include "utils/audio/cringe_audio.h"
#include "utils/audio/cringe_ffmpeg.h"
#include "utils/audio/cringe_youtube.h"
#include <array>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>
#include <opus/opus.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fmt/format.h>

CringeAudioStreamer::CringeAudioStreamer() : running_(true) {
    int error;
    opus_encoder_ = opus_encoder_create(OPUS_SAMPLE_RATE, OPUS_CHANNELS, OPUS_APPLICATION_AUDIO, &error);
    if (error != OPUS_OK) {
        throw std::runtime_error("Failed to create Opus encoder");
    }
    worker_thread_ = std::thread(&CringeAudioStreamer::process_audio_queue, this);
}

CringeAudioStreamer::~CringeAudioStreamer() {
    stop();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    if (opus_encoder_) {
        opus_encoder_destroy(opus_encoder_);
    }
}

bool CringeAudioStreamer::pause() {
    if (!is_paused_) {
        is_paused_ = true;
        return true;
    }
    return false;
}

bool CringeAudioStreamer::resume() {
    if (is_paused_) {
        is_paused_ = false;
        cv_.notify_all();
        return true;
    }
    return false;
}

void CringeAudioStreamer::set_volume(const float volume) {
    volume_ = std::clamp(volume, 0.0f, 2.0f);
}

void CringeAudioStreamer::set_speed(const float speed) {
    speed_ = std::clamp(speed, 0.5f, 2.0f);
}

void CringeAudioStreamer::set_bass_boost(const float boost) {
    bass_boost_ = std::clamp(boost, 0.0f, 2.0f);
}

void CringeAudioStreamer::set_loop_mode(const LoopMode mode) {
    loop_mode_ = mode;
}

void CringeAudioStreamer::clear_queue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!audio_queue_.empty()) {
        audio_queue_.pop();
    }
    cv_.notify_all();
}

void CringeAudioStreamer::skip() {
    std::cout << "\n=== Skip Initiated ===\n";
    should_skip_ = true;
    
    if (ytdlp_pid_ > 0) {
        std::cout << "Killing yt-dlp process: " << ytdlp_pid_ << "\n";
        kill(ytdlp_pid_, SIGKILL);
        waitpid(ytdlp_pid_, nullptr, 0);
        ytdlp_pid_ = 0;
    }
    
    if (ffmpeg_pid_ > 0) {
        std::cout << "Killing ffmpeg process: " << ffmpeg_pid_ << "\n";
        kill(ffmpeg_pid_, SIGKILL);
        waitpid(ffmpeg_pid_, nullptr, 0);
        ffmpeg_pid_ = 0;
    }

    std::cout << "Clearing queue and resetting state\n";
    clear_queue();
    current_stream_id_ = "";
    
    std::cout << "Notifying waiting threads\n";
    cv_.notify_all();
}

void CringeAudioStreamer::stop() {
    should_stop_ = true;
    running_ = false;
    skip(); // This will handle process cleanup
}

std::future<void> CringeAudioStreamer::stream(dpp::voiceconn* voice, const CringeSong& song, StreamCallback on_complete) {
    std::cout << "\n=== Starting Stream ===\n";
    std::cout << "Song: " << song.title << "\n";
    
    if (!voice || !voice->voiceclient || !voice->voiceclient->is_ready()) {
        std::cout << "Error: Voice connection not ready\n";
        throw std::runtime_error("Voice connection is not ready");
    }

    auto callback = std::make_shared<StreamCallback>(std::move(on_complete));
    should_stop_ = false;
    should_skip_ = false;

    std::cout << "Launching async stream task\n";
    return std::async(std::launch::async, [this, voice, callback, song]() {
        try {
            std::cout << "Setting up stream commands\n";
            std::string cmd;
            
            if (song.is_tts) {
                // Direct FFmpeg command for local files
                cmd = CringeFFMPEG::get_stream(
                    song.filter && !song.filter->empty() 
                        ? std::optional{std::vector<std::string>{CringeFFMPEG::get_filters().find(song.filter->at(0))->second}}
                        : std::nullopt,
                    song.url  // Pass the local file path directly
                );
            } else {
                // YouTube streaming command
                const auto filters = CringeFFMPEG::get_filters();
                std::string ytdlp_cmd = fmt::format("yt-dlp -o - -f bestaudio \"{}\"", song.url);
                std::string ffmpeg_cmd = CringeFFMPEG::get_stream(

                );
                cmd = fmt::format("{} | {}", ytdlp_cmd, ffmpeg_cmd);
            }
            
            std::cout << "Starting audio stream\n";
            stream_audio(voice, song, cmd);
            
            // Rest of your existing code...
        } catch (const std::exception& e) {
            std::cout << "Streaming error: " << e.what() << "\n";
        }
    });
}

void CringeAudioStreamer::stream_audio(dpp::voiceconn* voice, const CringeSong& song, const std::string& cmd) {
    if (!voice || !voice->voiceclient) return;

    setpgid(0, 0);

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to open pipe for audio streaming");
    }

    struct pollfd pfd;
    pfd.fd = fileno(pipe);
    pfd.events = POLLIN;

    constexpr size_t FRAME_SIZE = AudioChunk::PCM_FRAME_SIZE;
    std::vector<std::byte> buffer(FRAME_SIZE);

    const std::string stream_id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    current_stream_id_ = stream_id;

    voice->voiceclient->set_send_audio_type(dpp::discord_voice_client::satype_overlap_audio);

    while (!should_stop_ && !should_skip_) {
        if (is_paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        if (const int poll_result = poll(&pfd, 1, 10); poll_result > 0 && (pfd.revents & POLLIN)) {
            const size_t bytes_read = fread(buffer.data(), 1, FRAME_SIZE, pipe);
            if (bytes_read == 0) break;

            if (bytes_read == FRAME_SIZE) {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (current_stream_id_ == stream_id) {
                    audio_queue_.push({buffer, bytes_read, voice, false, 0, 0, stream_id});
                    cv_.notify_one();
                }
            }
        } else if (poll_result < 0) {
            break;
        }
    }

    pclose(pipe);
    voice->voiceclient->insert_marker("end");
}

void CringeAudioStreamer::process_audio_queue() {
    uint32_t current_timestamp = 0;

    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait(lock, [this] {
            return !audio_queue_.empty() || !running_ || should_skip_;
        });

        if (!running_) break;

        if (should_skip_) {
            clear_queue();
            should_skip_ = false;
            lock.unlock();
            continue;
        }

        while (!audio_queue_.empty()) {
            if (should_stop_ || should_skip_) break;

            auto chunk = std::move(audio_queue_.front());
            audio_queue_.pop();
            lock.unlock();

            if (chunk.voice && chunk.voice->voiceclient &&
                chunk.voice->voiceclient->is_ready()) {

                if (chunk.is_opus) {
                    chunk.voice->voiceclient->send_audio_opus(
                        reinterpret_cast<uint8_t*>(chunk.data.data()),
                        chunk.size,
                        20
                    );
                } else {
                    chunk.voice->voiceclient->send_audio_raw(
                        reinterpret_cast<uint16_t*>(chunk.data.data()),
                        chunk.size
                    );
                }
            }

            lock.lock();
        }
    }
}

