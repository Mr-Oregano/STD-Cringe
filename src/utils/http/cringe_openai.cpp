#include "utils/http/cringe_openai.h"
#include <fmt/core.h>


CringeOpenAI::CringeOpenAI(const std::string& api_key) : api_key(api_key) {}

std::string CringeOpenAI::sanitize(const std::string &prompt) {
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

json CringeOpenAI::chat(const std::string& model, const std::string& prompt, const std::string& system_prompt, const std::vector<std::pair<std::string, std::string>>& context) {
	json messages = json::array();
    // Add system message
    messages.push_back({
        {"role", "system"},
        {"content", "You are a helpful assistant."}
    });

    // Add conversation history
    for (const auto& [user_msg, assistant_msg] : context) {
        messages.push_back({
            {"role", "user"},
            {"content", user_msg}
        });
        messages.push_back({
            {"role", "assistant"},
            {"content", assistant_msg}
        });
    }

    // Add current prompt
    messages.push_back({
        {"role", "user"},
        {"content", sanitize(prompt)}
    });

	std::cout << sanitize(prompt);
    
    const json request_body = {
        {"model", model},
        {"messages", messages},
        {"temperature", 0.95}
    };

	const std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + api_key
    };

    try {
        json response = this->curl.post(
            request_body.dump(),
            base_url + "/chat/completions",
            headers
        );
        
        if (!response.contains("choices") || 
            response["choices"].empty() || 
            !response["choices"][0].contains("message") ||
            !response["choices"][0]["message"].contains("content")) {
            throw std::runtime_error("Invalid response format from OpenAI API");
        }

        return {
            {"response", response["choices"][0]["message"]["content"]}
        };
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("OpenAI API error: {}", e.what()));
    }
}

json CringeOpenAI::oneshot(const std::string& model, const std::string& prompt, const std::string& system_prompt) {
	json messages = json::array();
	return messages;
}

json CringeOpenAI::generate_image(const std::string& prompt) {
    json request_body = {
        {"prompt", sanitize(prompt)},
        {"n", 1},
        {"size", "1024x1024"}
    };

    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + api_key
    };

    try {
        json response = curl.post(
            request_body.dump(),
            base_url + "/images/generations",
            headers
        );

        if (!response.contains("data") || 
            response["data"].empty() || 
            !response["data"][0].contains("url")) {
            throw std::runtime_error("Invalid response format from OpenAI API");
        }

        return {
            {"response", response["data"][0]["url"]}
        };
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("OpenAI API error: {}", e.what()));
    }
}
