#include "utils/audio/cringe_song.h"
#include <iomanip>
#include <sstream>

std::string CringeSong::get_duration_string() const {
    if (!duration.has_value()) {
        return "Unknown duration";
    }
    
    auto total_seconds = duration.value().count();
    auto hours = total_seconds / 3600;
    auto minutes = (total_seconds % 3600) / 60;
    auto seconds = total_seconds % 60;
    
    std::stringstream ss;
    if (hours > 0) {
        ss << hours << ":";
    }
    ss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << seconds;
    
    return ss.str();
}

std::string CringeSong::get_view_count_string() const {
    if (!view_count.has_value()) {
        return "No views";
    }
    return format_number(view_count.value());
}

std::string CringeSong::format_number(uint64_t number) {
    const std::vector<std::pair<uint64_t, const char*>> units = {
        {1000000000, "B"},
        {1000000, "M"},
        {1000, "K"}
    };
    
    for (const auto& [threshold, suffix] : units) {
        if (number >= threshold) {
            double value = static_cast<double>(number) / threshold;
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << value << suffix;
            return ss.str();
        }
    }
    
    return std::to_string(number);
}