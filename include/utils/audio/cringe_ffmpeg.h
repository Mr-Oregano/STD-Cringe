#ifndef CRINGE_CRINGE_FFMPEG_H
#define CRINGE_CRINGE_FFMPEG_H

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <fmt/format.h>

class CringeFFMPEG {
public:
    CringeFFMPEG();
    ~CringeFFMPEG();

	static auto get_stream(const std::optional<std::vector<std::string>>& filters) -> std::string;
    static auto get_filters() -> const std::unordered_map<std::string, std::string>& {
        static const std::unordered_map<std::string, std::string> filters = {
		{"Nightcore", "aresample=48000,asetrate=48000*1.5"},
		{"Bassboost", "bass=g=9:f=160:w=0.8"},
		{"What the...", "aresample=48000,asetrate=48000,acrusher=level_in=2:level_out=4,bass=g=17:f=120:w=0.5"},
		{"Lofi Hip Hop Beats to Relax/Study to", "aresample=48000,asetrate=48000*0.9,extrastereo=m=2.5:c=disabled"},
		{"Widen Audio", "compand=attacks=0:points=-80/-169|-54/-80|-49.5/-64.6|-41.1/-41.1|-25.8/-15|-10.8/-4.5|0/0|20/8.3"},
		{"Sigma Type Beat", "aresample=48000,asetrate=48000*0.8,bass=g=8:f=110:w=0.6"},
		{"Paper Cut", "aexciter=level_in=3:level_out=3:amount=3:drive=8:blend=8,bass=g=2:f=100:w=0.6"},

        // New reverb effects
        {"Cathedral", "aecho=0.8:0.9:500|250:0.3|0.25|0.2,areverse,aecho=0.8:0.9:1000|500|250:0.3|0.25|0.2,areverse"},
        {"Small Room", "aecho=0.8:0.88:60|50:0.4|0.3"},
        {"Large Hall", "aecho=0.8:0.9:500|400|300|200:0.4|0.3|0.2|0.1"},

        // Complex effects
        {"Robot", "aecho=0.8:0.88:6|4:0.4|0.3,aphaser=type=t:speed=2:decay=0.6"},
        {"Alien", "vibrato=f=4:d=0.5,aecho=0.8:0.88:40|25:0.4|0.3,aphaser"},
        {"Underwater", "lowpass=f=800,aecho=0.8:0.9:40|25:0.8|0.7,areverse,aecho=0.8:0.9:40|25:0.8|0.7,areverse"},

        // Complex effects
        {"Dream", "aecho=0.8:0.9:60|40:0.4|0.3,highpass=f=200,lowpass=f=3000,areverse,aecho=0.8:0.9:60|40:0.4|0.3,areverse"},
        {"Radio", "bandpass=f=1500:width_type=h:width=1000,volume=2,equalizer=f=1500:width_type=h:width=1000:g=-10"},
        {"Phone", "highpass=f=500,lowpass=f=4000,volume=2,equalizer=f=2000:width_type=h:width=1000:g=6"},

        // Extreme effects
        {"Vinyl", "aresample=48000,aphaser=type=t:speed=0.6:decay=0.6,areverse,aphaser=type=t:speed=0.6:decay=0.6,areverse,lowpass=f=4000,highpass=f=20,acompressor"},
        {"Telephone", "highpass=f=500,lowpass=f=2000,areverse,aphaser,areverse"},

        {"Void", "aecho=0.8:1:1000|1800:0.7|0.5,areverse,aecho=0.8:1:1000|1800:0.7|0.5,areverse,lowpass=f=1000"},
        {"Glitch", "vibrato=f=8:d=1,tremolo=f=6:d=0.8,asetrate=48000*random(0.8,1.2):arandom"},

        {"8-bit", "aresample=8000,acrusher=bits=8:mode=hard:aa=yes"},
        // {"Underwater", "lowpass=f=800,aphaser=type=t:speed=2:decay=0.6"},
        {"Robot", "aresample=48000,aphaser=type=s:speed=2:decay=0.6,aecho=0.8:0.88:8:0.8"},
        // {"Chipmunk", "aresample=48000,asetrate=48000*1.4,aecho=0.8:0.8:10:0.5"},
        {"Demon", "aresample=48000,asetrate=48000*0.75,aecho=0.8:0.8:10:0.5"},
        // {"Concert Hall", "aecho=0.8:0.9:50|100|150:0.4|0.3|0.2,areverse,aecho=0.8:0.9:50|100|150:0.4|0.3|0.2,areverse"},
        // {"Radio", "highpass=f=200,lowpass=f=3000,aphaser,volume=2"},
        {"Reverse", "areverse"},
        // {"Tremolo", "tremolo=f=10:d=0.8"},
        // {"Vibrato", "vibrato=f=7:d=0.5"},
        // {"Double Track", "adelay=42|42,aecho=0.8:0.8:10:0.5"},
        // {"Flanger", "flanger=speed=5:depth=2:regen=5:width=71:shape=sinusoidal:phase=25:interp=linear"},
        {"Harmonizer", "aresample=48000,apulsator=mode=sine:hz=0.5,aecho=0.8:0.8:60:0.4"}
    };
        return filters;
    }
    static std::vector<const char*> get_stream_args(const std::optional<std::vector<std::string>>& filters);
    static std::string get_stream(const std::optional<std::vector<std::string>>& filters = std::nullopt, 
                                const std::string& input = "-") {
        std::string filter_str = filters ? fmt::format("-af \"{}\"", fmt::join(*filters, ",")) : "";
        return fmt::format("ffmpeg -i {} {} -f s16le -ar 48000 -ac 2 pipe:1", 
                          input == "-" ? "pipe:0" : fmt::format("\"{}\"", input), 
                          filter_str);
    }

private:
    static constexpr const char* BASE_COMMAND = 
        "ffmpeg -i pipe:0 -hide_banner -loglevel warning -af \"aresample=48000:async=1000,{}\" -f s16le -frame_size 960 -ac 2 -ar 48000 pipe:1";
    static constexpr const char* FFMPEG_BASE_ARGS[] = {
        "ffmpeg",
        "-i", "pipe:0",
        "-f", "s16le",
        "-ar", "48000",
        "-ac", "2",
        "-acodec", "pcm_s16le"
    };
    static auto set_filter(const std::string& filter) -> std::string;
    [[nodiscard]] static auto validate_filter(const std::string& filter) -> bool;
    static auto combine_filters(const std::vector<std::string>& filter_names) -> std::string;
};

#endif
