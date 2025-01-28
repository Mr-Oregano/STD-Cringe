#ifndef CRINGE_CRINGE_OLLAMA_H
#define CRINGE_CRINGE_OLLAMA_H

#include <dpp/dpp.h>
#include <iostream>
#include <string>
#include <fmt/core.h>
#include <regex>
#include <curl/curl.h>
#include "cringe_curl.h"

class CringeOllama {
public:
	CringeOllama(const std::string &endpoint);
	~CringeOllama();
	json chat(const std::string &model, const std::string &prompt, const std::string& system_prompt = "", const std::vector<std::pair<std::string, std::string>>& context = {});
	json oneshot(const std::string& model, const std::string& prompt, const std::string& system_prompt = "");
	json list();
private:
	CringeCurl curl;
	std::string endpoint;
	static std::string sanitize(const std::string &prompt);
};

#endif
