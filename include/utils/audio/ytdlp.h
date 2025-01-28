// jan 8

#pragma once
#include <string>
#include <vector>
#include "utils/audio/track.h"


class YTDLPWrapper {
public:
    static std::string get_stream_url(const std::string& url);
    static Track get_track_info(const std::string& url, 
                              const std::string& requester_name,
                              dpp::snowflake requester_id);
    static std::vector<Track> search(const std::string& query, size_t limit = 5);
    static std::vector<Track> get_playlist(const std::string& url);

private:
    static std::string exec(const std::string& cmd);
};
