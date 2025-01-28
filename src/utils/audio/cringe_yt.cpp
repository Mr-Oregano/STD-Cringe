#include "utils/audio/cringe_youtube.h"
#include "utils/misc/cringe_pipe.h"

CringeYoutube::CringeYoutube() = default;
CringeYoutube::~CringeYoutube() = default;

CringeSong CringeYoutube::get_content(const std::string& query) {
    std::string sanitized = sanitize(query);
    if (!is_url(sanitized)) {
        sanitized = to_url(sanitized);
    }
    
    std::string cmd = fmt::format("yt-dlp -s --print title --print channel --print thumbnail --print duration_string --print view_count --print comment_count --print release_timestamp --print channel_follower_count --print description --print webpage_url \"{}\"", sanitized);
    std::cout << fmt::format(YT_DLP_INFO_CMD, sanitized) << "\n";
    
    // Use simpler command execution
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute command");
    }

    std::array<char, 128> buffer;
    std::string result;
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    
    CringeSong song;
    parse_song_info(result, song);
    song.url = sanitized;
	std::cout << "SENDING BACK SONG" + song.url + "\n";
    return song;
}

std::string CringeYoutube::to_url(const std::string& query) {
    std::string command = fmt::format(YT_DLP_SEARCH_CMD, query);
    std::string video_id = execute_command(command);
    
    if (video_id.empty()) {
        throw std::runtime_error("Failed to get URL from search query");
    }
    
    // Remove trailing newline if present
    if (video_id.back() == '\n') {
        video_id.pop_back();
    }
    
    // Construct proper YouTube URL
    return fmt::format("https://youtube.com/watch?v={}", video_id);
}

std::string CringeYoutube::search(const std::string& query) {
    std::string sanitized = sanitize(query);
    std::string video_id;
    
    if (is_url(sanitized)) {
        video_id = extract_video_id(sanitized);
    } else {
        try {
            video_id = to_url(sanitized);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to find video: " + std::string(e.what()));
        }
    }
    
    if (video_id.empty()) {
        throw std::runtime_error("Could not extract video ID");
    }

	std::cout << "IN DA SEARCH COMMAND\n";
	const std::string commd = fmt::format(YT_DLP_STREAM_CMD, fmt::format("{}", video_id));
	std::cout << "COMMAND: " + commd + "\n";
    
    // Format URL properly for yt-dlp
    return fmt::format(YT_DLP_STREAM_CMD, fmt::format("{}", video_id));
}

bool CringeYoutube::is_url(const std::string& query) const {
    static const std::regex url_patterns[] = {
        std::regex(R"(^(https?|ftp):\/\/[^\s/$.?#].[^\s]*)")
    };
    
    for (const auto& pattern : url_patterns) {
        if (std::regex_match(query, pattern)) {
            return true;
        }
    }
    return false;
}

std::string CringeYoutube::extract_video_id(const std::string& url) const {
	static const std::regex patterns[] = {
			std::regex(R"(v=([a-zA-Z0-9_-]+))"),         // Standard YouTube URL
			std::regex(R"(youtu\.be/([a-zA-Z0-9_-]+))"), // Short YouTube URL
			std::regex(R"(embed/([a-zA-Z0-9_-]+))")      // Embedded YouTube URL
	};

	std::smatch match;
	for (const auto& pattern : patterns) {
		if (std::regex_search(url, match, pattern) && match.size() > 1) {
			return match[1].str();
		}
	}
	return url;
}

bool CringeYoutube::is_playlist(const std::string& query) const {
    static const std::regex playlist_regex("&(start_radio=|list=)");
    return std::regex_search(query, playlist_regex);
}

std::string CringeYoutube::sanitize(std::string query) const {
    // Only remove the most dangerous characters while preserving URL structure
    query.erase(std::remove_if(query.begin(), query.end(),
        [](char c) { 
            return iscntrl(c) || c == '`' || c == '$';
        }),
        query.end());
    return query;
}

std::string CringeYoutube::execute_command(const std::string& cmd) const {
    std::string sanitized_cmd = sanitize_command(cmd);
    FILE* pipe = popen(sanitized_cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute command: " + sanitized_cmd);
    }
    
    PipeGuard guard(pipe);
    std::array<char, BUFFER_SIZE> buffer;
    std::string result;
    result.reserve(BUFFER_SIZE);
    
    bool has_content = false;
    while (fgets(buffer.data(), buffer.size(), guard.get()) != nullptr) {
        std::string line(buffer.data());
        // Skip error lines that start with "ERROR:"
        if (line.find("ERROR:") == std::string::npos) {
            result += line;
            has_content = true;
        }
    }
    
    if (!has_content) {
        throw std::runtime_error("No valid output from command");
    }
    
    return result;
}

void CringeYoutube::parse_song_info(const std::string& result, CringeSong& song) const {
    std::istringstream stream(result);
    std::string line;

    // Required fields
    if (!std::getline(stream, song.title) || song.title.empty()) {
        throw std::runtime_error("Failed to parse song title");
    }

    // Optional fields
    std::getline(stream, line);
    song.artist = line.empty() ? std::nullopt : std::optional<std::string>(line);

    std::getline(stream, line);
    song.thumbnail = line.empty() ? std::nullopt : std::optional<std::string>(line);

    // Parse duration
    std::getline(stream, line);
    if (!line.empty()) {
        try {
            // Parse duration string (format: HH:MM:SS or MM:SS)
            std::istringstream duration_stream(line);
            int hours = 0, minutes = 0, seconds = 0;
            char delimiter;
            
            if (line.find(':') != line.rfind(':')) {
                duration_stream >> hours >> delimiter >> minutes >> delimiter >> seconds;
            } else {
                duration_stream >> minutes >> delimiter >> seconds;
            }
            
            song.duration = std::chrono::seconds(hours * 3600 + minutes * 60 + seconds);
        } catch (...) {
            song.duration = std::nullopt;
        }
    }

    // Parse view count
    std::getline(stream, line);
    if (!line.empty()) {
        try {
            song.view_count = std::stoull(line);
        } catch (...) {
            song.view_count = std::nullopt;
        }
    }

    // Parse comment count
    std::getline(stream, line);
    if (!line.empty()) {
        try {
            song.comment_count = std::stoull(line);
        } catch (...) {
            song.comment_count = std::nullopt;
        }
    }

    // Parse upload date
    std::getline(stream, line);
    if (!line.empty()) {
        try {
            std::time_t timestamp = std::stoull(line);
            song.upload_date = std::chrono::system_clock::from_time_t(timestamp);
        } catch (...) {
            song.upload_date = std::nullopt;
        }
    }

    // Parse subscriber count
    std::getline(stream, line);
    if (!line.empty()) {
        try {
            song.subscriber_count = std::stoull(line);
        } catch (...) {
            song.subscriber_count = std::nullopt;
        }
    }

    // Parse description and channel URL
    std::getline(stream, line);
    song.description = line.empty() ? std::nullopt : std::optional<std::string>(line);
    
    std::getline(stream, line);
    song.channel_url = line.empty() ? std::nullopt : std::optional<std::string>(line);
}