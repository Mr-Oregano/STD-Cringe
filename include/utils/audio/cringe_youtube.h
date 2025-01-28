#ifndef CRINGE_CRINGE_YOUTUBE_H
#define CRINGE_CRINGE_YOUTUBE_H

#include <string>
#include <array>
#include <dpp/dpp.h>
#include <algorithm>
#include <regex>
#include <fmt/format.h>
#include "utils/audio/cringe_song.h"
#include <sstream>
#include <stdexcept>
#include "utils/misc/cringe_async.h"

class CringeYoutube {
public:
    CringeYoutube();
    ~CringeYoutube();

    Task<CringeSong> get_content_async(const std::string& query);
    Task<std::string> search_async(const std::string& query);
    
    // Keep old methods for backward compatibility
    CringeSong get_content(const std::string& query);
    std::string search(const std::string& query);

private:
    static constexpr size_t BUFFER_SIZE = 128;
    
    static constexpr const char* YT_DLP_INFO_CMD = 
        "yt-dlp --no-warnings --force-ipv4 --extract-audio --print \"%(title)s\\n%(uploader)s\\n%(thumbnail)s\\n"
        "%(duration)s\\n%(view_count)s\\n%(comment_count)s\\n%(upload_date)s\\n%(channel_follower_count)s\\n"
        "%(description)s\\n%(webpage_url)s\" \"{}\"";
    
    static constexpr const char* YT_DLP_SEARCH_CMD = 
        R"(yt-dlp --no-warnings --get-id "ytsearch1:{}")";

    static constexpr const char* YT_DLP_STREAM_CMD = 
        R"(yt-dlp -o - -vn "{}")";

    Task<std::string> execute_command_async(const std::string& cmd) const;
    std::string execute_command(const std::string& cmd) const;
    std::string to_url(const std::string& query);
    Task<std::string> to_url_async(const std::string& query);
    bool is_url(const std::string& query) const;
    bool is_playlist(const std::string& query) const;
    std::string search_command(const std::string& search) const;
    std::string sanitize(std::string query) const;
    std::string extract_video_id(const std::string& url) const;
    void parse_song_info(const std::string& result, CringeSong& song) const;
};

#endif