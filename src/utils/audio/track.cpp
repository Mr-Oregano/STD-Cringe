// Jan 8

#include "utils/audio/track.h"
#include "utils/audio/ytdlp.h"
#include <fmt/format.h>
#include <regex>

Track::Track(const std::string& url, 
            const std::string& title, 
            const std::string& duration,
            const std::string& requester_name,
            dpp::snowflake requester_id) {
    info.url = url;
    info.title = title;
    info.duration = duration;
    info.requester_name = requester_name;
    info.requester_id = requester_id;
}

std::string Track::get_stream_url() const {
    return YTDLPWrapper::get_stream_url(info.url);
}

std::chrono::seconds Track::parse_duration() const {
    std::regex time_pattern(R"((\d+):(\d+))");
    std::smatch matches;
    
    // if (std::regex_match(info.duration, matches)) {
    //     int minutes = std::stoi(matches[1].str());
    //     int seconds = std::stoi(matches[2].str());
    //     return std::chrono::seconds(minutes * 60 + seconds);
    // }
    
    return std::chrono::seconds(0);
}
