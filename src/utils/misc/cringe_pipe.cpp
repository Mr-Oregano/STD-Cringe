#include "utils/misc/cringe_pipe.h"

bool is_safe_command(const std::string& cmd) {
    static const std::regex safe_pattern(R"(^[a-zA-Z0-9\s\-_./'":]+$)");
    return std::regex_match(cmd, safe_pattern);
}

std::string sanitize_command(const std::string& cmd) {
    std::string result;
    result.reserve(cmd.length());
    
    for (char c : cmd) {
        if (std::isalnum(c) || c == ' ' || c == '-' || c == '_' || 
            c == '.' || c == '/' || c == '\'' || c == '"' || c == ':') {
            result += c;
        }
    }
    return result;
}