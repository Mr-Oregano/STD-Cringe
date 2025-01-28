// Jan 8
#ifndef CRINGE_TRACK_H
#define CRINGE_TRACK_H
#include <dpp/dpp.h>
#include <string>
#include <chrono>
#include <optional>

struct TrackInfo {
    std::string url;
    std::string title;
    std::string duration;
    std::string requester_name;
    dpp::snowflake requester_id;
    std::optional<std::string> thumbnail_url;
    std::optional<uint64_t> view_count;
    std::optional<std::string> channel_name;
};

class Track {
public:
    Track(const std::string& url, 
          const std::string& title, 
          const std::string& duration,
          const std::string& requester_name = "",
          dpp::snowflake requester_id = 0);

    // Core functionality
    std::string get_stream_url() const;
    std::chrono::seconds parse_duration() const;
    
    // Getters
    const std::string& get_url() const { return info.url; }
    const std::string& get_title() const { return info.title; }
    const std::string& get_duration() const { return info.duration; }
    const std::string& get_requester_name() const { return info.requester_name; }
    dpp::snowflake get_requester_id() const { return info.requester_id; }
    
    // Optional metadata
    std::optional<std::string> get_thumbnail_url() const { return info.thumbnail_url; }
    std::optional<uint64_t> get_view_count() const { return info.view_count; }
    std::optional<std::string> get_channel_name() const { return info.channel_name; }
    
    void set_thumbnail_url(const std::string& url) { info.thumbnail_url = url; }
    void set_view_count(uint64_t count) { info.view_count = count; }
    void set_channel_name(const std::string& name) { info.channel_name = name; }

private:
    TrackInfo info;
};

#endif