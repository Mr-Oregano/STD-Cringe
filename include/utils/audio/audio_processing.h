// Jan 8
#pragma once
#include <string>
#include <functional>
#include <memory>
#include <array>
#include <thread>
#include <atomic>
#include <opus/opus.h>

class AudioProcessor {
public:
    using AudioCallback = std::function<bool(const uint8_t*, size_t)>;
    static constexpr size_t PCM_FRAME_SIZE = 960 * 2 * 2; // 960 samples * 2 channels * 2 bytes per sample
    static constexpr int OPUS_SAMPLE_RATE = 48000;
    static constexpr int OPUS_CHANNELS = 2;
    static constexpr int OPUS_FRAME_SIZE = 960;

    AudioProcessor();
    ~AudioProcessor();

    void process_stream(const std::string& ffmpeg_command, AudioCallback callback);
    void stop();

    // Audio effect controls
    void set_volume(float volume);
    void set_speed(float speed);
    void set_bass_boost(float boost);

private:
    std::atomic<bool> should_stop{false};
    std::unique_ptr<FILE, decltype(&pclose)> pipe;
    OpusEncoder* opus_encoder{nullptr};
    
    std::atomic<float> volume_{1.0f};
    std::atomic<float> speed_{1.0f};
    std::atomic<float> bass_boost_{1.0f};

    bool start_ffmpeg_process(const std::string& command);
    void read_audio_data(AudioCallback callback);
    void apply_effects(int16_t* buffer, size_t size);
    static std::string sanitize_command(const std::string& command);
};
