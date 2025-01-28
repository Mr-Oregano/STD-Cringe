#ifndef CRINGE_CRINGE_SONG_H
#define CRINGE_CRINGE_SONG_H

#include <chrono>
#include <dpp/dpp.h>
#include <optional>
#include <string>

class CringeSong {
public:
    // Required fields
    std::string title;
    std::string url;
    
    // Optional fields
    std::optional<std::string> artist;
    std::optional<std::string> thumbnail;
    std::optional<std::chrono::seconds> duration;
    std::optional<uint64_t> view_count;
    std::optional<uint64_t> comment_count;
    std::optional<std::chrono::system_clock::time_point> upload_date;
    std::optional<uint64_t> subscriber_count;
    std::optional<std::string> description;
    std::optional<std::string> channel_url;
    std::optional<std::vector<std::string>> filter;
    bool is_tts = false; 

    // Custom serialization for std::optional
    friend void to_json(nlohmann::json& j, const CringeSong& song) {
        j = nlohmann::json{
            {"title", song.title},
            {"url", song.url}
        };

        // Optional fields - only add if they have values
        if (song.artist) j["artist"] = *song.artist;
        if (song.thumbnail) j["thumbnail"] = *song.thumbnail;
        if (song.duration) j["duration"] = song.duration->count();
        if (song.view_count) j["view_count"] = *song.view_count;
        if (song.comment_count) j["comment_count"] = *song.comment_count;
        if (song.upload_date) j["upload_date"] = std::chrono::system_clock::to_time_t(*song.upload_date);
        if (song.subscriber_count) j["subscriber_count"] = *song.subscriber_count;
        if (song.description) j["description"] = *song.description;
        if (song.channel_url) j["channel_url"] = *song.channel_url;
        if (song.filter) j["filter"] = *song.filter;
    }

    friend void from_json(const nlohmann::json& j, CringeSong& song) {
        // Required fields
        j.at("title").get_to(song.title);
        j.at("url").get_to(song.url);

        // Optional fields - only get if they exist and aren't null
        if (j.contains("artist") && !j["artist"].is_null())
            song.artist = j["artist"].get<std::string>();
        if (j.contains("thumbnail") && !j["thumbnail"].is_null())
            song.thumbnail = j["thumbnail"].get<std::string>();
        if (j.contains("duration") && !j["duration"].is_null())
            song.duration = std::chrono::seconds(j["duration"].get<int64_t>());
        if (j.contains("view_count") && !j["view_count"].is_null())
            song.view_count = j["view_count"].get<uint64_t>();
        if (j.contains("comment_count") && !j["comment_count"].is_null())
            song.comment_count = j["comment_count"].get<uint64_t>();
        if (j.contains("upload_date") && !j["upload_date"].is_null())
            song.upload_date = std::chrono::system_clock::from_time_t(j["upload_date"].get<time_t>());
        if (j.contains("subscriber_count") && !j["subscriber_count"].is_null())
            song.subscriber_count = j["subscriber_count"].get<uint64_t>();
        if (j.contains("description") && !j["description"].is_null())
            song.description = j["description"].get<std::string>();
        if (j.contains("channel_url") && !j["channel_url"].is_null())
            song.channel_url = j["channel_url"].get<std::string>();
        if (j.contains("filter") && !j["filter"].is_null())
            song.filter = j["filter"].get<std::vector<std::string>>();
    }

    // Helper methods
    std::string serialize() const {
        return nlohmann::json(*this).dump();
    }

    static CringeSong deserialize(const std::string& json_str) {
        return nlohmann::json::parse(json_str).get<CringeSong>();
    }

    // Helper methods remain the same
    std::string get_duration_string() const;
    std::string get_view_count_string() const;


	bool has_filter() const { return filter.has_value() && !filter->empty(); }

private:
	static std::string format_number(uint64_t number);
};

#endif // CRINGE_CRINGE_SONG_H
