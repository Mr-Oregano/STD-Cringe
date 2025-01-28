#include "utils/audio/audio_processor.h"
#include <algorithm>
#include <cstring>
#include <fmt/format.h>
#include <poll.h>

AudioProcessor::AudioProcessor() 
    : pipe(nullptr, pclose) {
    int error;
    opus_encoder = opus_encoder_create(OPUS_SAMPLE_RATE, OPUS_CHANNELS, OPUS_APPLICATION_AUDIO, &error);
    if (error != OPUS_OK) {
        throw std::runtime_error("Failed to create Opus encoder");
    }
}

AudioProcessor::~AudioProcessor() {
    stop();
    if (opus_encoder) {
        opus_encoder_destroy(opus_encoder);
    }
}

void AudioProcessor::process_stream(const std::string& ffmpeg_command, AudioCallback callback) {
    if (!start_ffmpeg_process(ffmpeg_command)) {
        throw std::runtime_error("Failed to start FFmpeg process");
    }

    read_audio_data(callback);
}

void AudioProcessor::stop() {
    should_stop = true;
    pipe.reset();
}

bool AudioProcessor::start_ffmpeg_process(const std::string& command) {
    std::string safe_command = sanitize_command(command);
    pipe.reset(popen(safe_command.c_str(), "r"));
    return pipe != nullptr;
}

void AudioProcessor::read_audio_data(AudioCallback callback) {
    pollfd pfd{};
    pfd.fd = fileno(pipe.get());
    pfd.events = POLLIN;

    std::vector<uint8_t> buffer(PCM_FRAME_SIZE);
    
    while (!should_stop) {
        int poll_result = poll(&pfd, 1, 100);
        if (poll_result > 0 && (pfd.revents & POLLIN)) {
            size_t bytes_read = fread(buffer.data(), 1, PCM_FRAME_SIZE, pipe.get());
            if (bytes_read == 0) break;

            if (bytes_read == PCM_FRAME_SIZE) {
                // Apply audio effects
                apply_effects(reinterpret_cast<int16_t*>(buffer.data()), bytes_read / 2);
                
                // Send to callback
                if (!callback(buffer.data(), bytes_read)) {
                    break;
                }
            }
        } else if (poll_result < 0) {
            break;
        }
    }
}

void AudioProcessor::apply_effects(int16_t* buffer, size_t size) {
    float current_volume = volume_.load();
    float current_speed = speed_.load();
    float current_bass = bass_boost_.load();
    
    // Apply volume
    if (current_volume != 1.0f) {
        for (size_t i = 0; i < size; i++) {
            float sample = buffer[i] * current_volume;
            buffer[i] = std::clamp(static_cast<int>(sample), INT16_MIN, INT16_MAX);
        }
    }
    
    // Apply bass boost using a simple low-pass filter
    if (current_bass != 1.0f) {
        static float prev_sample = 0.0f;
        float alpha = 0.2f * current_bass;
        
        for (size_t i = 0; i < size; i++) {
            float current = buffer[i];
            float filtered = alpha * current + (1.0f - alpha) * prev_sample;
            prev_sample = filtered;
            
            buffer[i] = std::clamp(static_cast<int>(filtered), INT16_MIN, INT16_MAX);
        }
    }
}

std::string AudioProcessor::sanitize_command(const std::string& command) {
    // Remove dangerous characters while preserving necessary command structure
    std::string result = command;
    result.erase(std::remove_if(result.begin(), result.end(),
        [](char c) { return iscntrl(c) || c == '`' || c == '$'; }),
        result.end());
    return result;
}

void AudioProcessor::set_volume(float new_volume) {
    volume_.store(std::clamp(new_volume, 0.0f, 2.0f));
}

void AudioProcessor::set_speed(float new_speed) {
    speed_.store(std::clamp(new_speed, 0.5f, 2.0f));
}

void AudioProcessor::set_bass_boost(float new_boost) {
    bass_boost_.store(std::clamp(new_boost, 0.0f, 2.0f));
}
