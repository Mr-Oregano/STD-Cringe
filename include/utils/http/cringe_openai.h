#ifndef CRINGE_OPENAI_H
#define CRINGE_OPENAI_H

#include <string>
#include <dpp/dpp.h>
#include "cringe_curl.h"

using json = nlohmann::json;

class CringeOpenAI {
private:
    CringeCurl curl;
    std::string api_key;
    std::string base_url = "https://api.openai.com/v1";
    static std::string sanitize(const std::string& prompt);

public:
    explicit CringeOpenAI(const std::string& api_key);
	json chat(const std::string& model, const std::string& prompt, const std::string& system_prompt = "", const std::vector<std::pair<std::string, std::string>>& context = {});
	json oneshot(const std::string& model, const std::string& prompt, const std::string& system_prompt = "");
	json generate_image(const std::string& prompt);
};

#endif