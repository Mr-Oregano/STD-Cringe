// Jan 8
#include "utils/audio/ytdlp.h"
#include <stdexcept>
#include <memory>
#include <array>
#include <sstream>
#include <fmt/format.h>


std::string YTDLPWrapper::get_stream_url(const std::string& url) {
    std::string cmd = fmt::format("yt-dlp -f bestaudio -g \"{}\"", url);
    std::string result = exec(cmd);
    
    // Remove trailing newline
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }
    
    if (result.empty()) {
        throw std::runtime_error("Failed to get stream URL");
    }
    
    return result;
}

Track YTDLPWrapper::get_track_info(const std::string& url,
                                 const std::string& requester_name,
                                 dpp::snowflake requester_id) {
    std::string cmd = fmt::format(
        "yt-dlp --get-title --get-duration \"{}\"",
        url
    );
    
    std::string result = exec(cmd);
    std::stringstream ss(result);
    std::string title, duration;
    
    std::getline(ss, title);
    std::getline(ss, duration);
    
    return Track(url, title, duration, requester_name, requester_id);
}

std::vector<Track> YTDLPWrapper::search(const std::string& query, size_t limit) {
    std::string cmd = fmt::format(
        "yt-dlp ytsearch{}:\"{}\" --get-title --get-duration --get-url -f bestaudio",
        limit,
        query
    );
    
    std::string result = exec(cmd);
    std::stringstream ss(result);
    std::vector<Track> tracks;
    
    std::string title, duration, url;
    while (std::getline(ss, title) && 
           std::getline(ss, duration) && 
           std::getline(ss, url)) {
        tracks.emplace_back(url, title, duration, "", 0);
    }
    
    return tracks;
}

std::vector<Track> YTDLPWrapper::get_playlist(const std::string& url) {
    std::string cmd = fmt::format(
        "yt-dlp --flat-playlist --get-title --get-duration --get-url \"{}\" -f bestaudio",
        url
    );
    
    std::string result = exec(cmd);
    std::stringstream ss(result);
    std::vector<Track> tracks;
    
    std::string title, duration, track_url;
    while (std::getline(ss, title) && 
           std::getline(ss, duration) && 
           std::getline(ss, track_url)) {
        tracks.emplace_back(track_url, title, duration, "", 0);
    }
    
    return tracks;
}

std::string YTDLPWrapper::exec(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}