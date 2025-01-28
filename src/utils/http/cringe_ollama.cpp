#include "utils/http/cringe_ollama.h"

struct OllamaChat {
	std::string model;
	std::string prompt;
};

CringeOllama::CringeOllama(const std::string &endpoint) : endpoint(endpoint) {}

CringeOllama::~CringeOllama() = default;

json CringeOllama::chat(const std::string &model, const std::string &prompt, const std::string& system_prompt, const std::vector<std::pair<std::string, std::string>>& context) {
	const std::string url = fmt::format("{}/api/chat", endpoint);
    std::string sanitized = this->sanitize(prompt);
    
    json messages = json::array();
    
    // Add system message if present
    if (!system_prompt.empty()) {
        messages.push_back({
            {"role", "system"},
            {"content", system_prompt}
        });
    }
    
    // Add conversation history
    for (const auto& [user_msg, assistant_msg] : context) {
        // Add user message
        messages.push_back({
            {"role", "user"},
            {"content", user_msg}
        });
        // Add assistant response
        messages.push_back({
            {"role", "assistant"},
            {"content", assistant_msg}
        });
    }
    
    // Add current user message
    messages.push_back({{"role", "user"}, {"content", prompt}});

	const json request_body = {
        {"model", model},
        {"messages", messages},
        {"stream", false}
    };
    
    const std::vector<std::string> headers = {
        "Content-Type: application/json"
    };
    
    try {
        json response = this->curl.post(request_body.dump(), url, headers);
        
        if (response.is_null()) {
            throw std::runtime_error("Empty response from Ollama API");
        }
        
        if (!response.is_object()) {
            throw std::runtime_error("Response is not a JSON object");
        }
        
        if (!response.contains("message")) {
            throw std::runtime_error("Response missing 'message' field");
        }
        
        if (response["message"].is_null()) {
            throw std::runtime_error("Message field is null");
        }
        
        // Extract the content from the message
        return {{"response", response["message"]["content"]}};
        
    } catch (const json::exception& e) {
        throw std::runtime_error(fmt::format("JSON parsing error: {}", e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("Ollama API error: {}", e.what()));
    }
}

json CringeOllama::oneshot(const std::string &model, const std::string &prompt, const std::string& system_prompt) {
	const std::string url = fmt::format("{}/api/generate", endpoint);
    std::string sanitized = this->sanitize(prompt);

    const json request = {
    	{"model", model},
    	{"prompt", prompt},
    	{"stream", false}
    };

    const std::vector<std::string> headers = {
        "Content-Type: application/json"
    };

	std::cout << std::setw(4) << request << std::endl;

    try {
        json response = this->curl.post(request.dump(), url, headers);

        if (response.is_null()) {
            throw std::runtime_error("Empty response from Ollama API");
        }

        if (!response.is_object()) {
            throw std::runtime_error("Response is not a JSON object");
        }

        if (!response.contains("response")) {
            throw std::runtime_error("Response missing 'message' field");
        }

        if (response["response"].is_null()) {
            throw std::runtime_error("Message field is null");
        }

        // Extract the content from the message
        return {{"response", response["response"]}};

    } catch (const json::exception& e) {
        throw std::runtime_error(fmt::format("JSON parsing error: {}", e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("Ollama API error: {}", e.what()));
    }
}

json CringeOllama::list() {
	const std::string url = fmt::format("{}/api/tags", endpoint);
	json response = this->curl.get(url);
	return response;
}

std::string CringeOllama::sanitize(const std::string &prompt) {
    std::string sanitized = prompt;
    
    // Replace newlines with spaces
    std::replace(sanitized.begin(), sanitized.end(), '\n', ' ');
    
    // Escape double quotes
    size_t found = sanitized.find('"');
    while (found != std::string::npos) {
        std::string replace = fmt::format(R"(\{})", '"');
        sanitized.replace(found, 1, replace);
        found = sanitized.find('\"', found + 2);
    }
    
    return sanitized;
}
