#include "utils/audio/cringe_ffmpeg.h"

CringeFFMPEG::CringeFFMPEG() = default;
CringeFFMPEG::~CringeFFMPEG() = default;

auto CringeFFMPEG::set_filter(const std::string& filter) -> std::string {
	auto filters = get_filters();

	if (!validate_filter(filter)) {
		throw std::invalid_argument("Invalid audio filter specified");
	}

	const auto cringe_filter = filters.find(filter);
	return fmt::format("-filter_complex {}", cringe_filter->second);
}

auto CringeFFMPEG::validate_filter(const std::string& filter) -> bool {
	const auto filters = get_filters();
	if (filter.empty()) {
		return true;
	}
	return filters.contains(filter);
}

auto CringeFFMPEG::get_stream_args(const std::optional<std::vector<std::string>>& filters) -> std::vector<const char*> {
    std::vector<const char*> args;
    
    // Add base arguments
    for (const char* arg : FFMPEG_BASE_ARGS) {
        args.push_back(arg);
    }

    // Add filters if present
    if (filters && !filters->empty()) {
        args.push_back("-af");
        
        // Join filters with comma
        static std::string filter_string;  // Static to ensure string lives long enough
        filter_string = "";
        for (size_t i = 0; i < filters->size(); ++i) {
            filter_string += (*filters)[i];
            if (i < filters->size() - 1) {
                filter_string += ",";
            }
        }
        args.push_back(filter_string.c_str());
    }

    // Add output format
    args.push_back("pipe:1");
    
    // Add null terminator for execvp
    args.push_back(nullptr);
    
    return args;
}

auto CringeFFMPEG::get_stream(const std::optional<std::vector<std::string>>& filters) -> std::string {
    std::string cmd = "ffmpeg -i pipe:0 -f s16le -ar 48000 -ac 2 -acodec pcm_s16le";
    
    if (filters && !filters->empty()) {
        cmd += " -af \"";
        for (size_t i = 0; i < filters->size(); ++i) {
            cmd += (*filters)[i];
            if (i < filters->size() - 1) {
                cmd += ",";
            }
        }
        cmd += "\"";
    }
    
    cmd += " pipe:1";
    return cmd;
}

auto CringeFFMPEG::combine_filters(const std::vector<std::string>& filter_names) -> std::string {
    std::vector<std::string> filter_chains;
    const auto& all_filters = get_filters();
    
    for (const auto& name : filter_names) {
        if (auto iter = all_filters.find(name); iter != all_filters.end()) {
            filter_chains.push_back(iter->second);
        }
    }
    
    if (filter_chains.empty()) {
    	return "";
    }
    
    // Join filters with comma and wrap in quotes
    return fmt::format("-filter_complex \"{}\"", fmt::join(filter_chains, ","));
}