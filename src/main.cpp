/*
 * MIT License
 *
 * Copyright (c) 2023 @nulzo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "utils/bot/cringe_bot.h"
#include "utils/misc/cringe_environment.h"
#include "utils/database/cringe_database.h"
#include "utils/database/queue_store.h"

const std::string OLLAMA_URL = "http://192.168.0.25:11434";

struct Config {
    std::string token;
    std::string guild;
	std::string ollama;
    std::string openai_key;
	std::string system_prompt;
};

Config load_config() {
    Config config;
    
    // Load required bot token
    config.token = get_env("NEW_BOT_TOKEN");
    if (config.token.empty()) {
        throw std::runtime_error("NEW_BOT_TOKEN not found in .env file");
    }
    
    // Load optional OpenAI key
    config.openai_key = get_env("OPEN_AI_AUTH");

	// Load the environment variables for the system prompt
	config.system_prompt = get_env("SYSTEM_PROMPT");
    
    // Load optional guild ID
    config.guild = get_env("GUILD");
    
    // Load Ollama endpoint (with fallback to default)
    config.ollama = get_env("OLLAMA_ENDPOINT");
    if (config.ollama.empty()) {
        config.ollama = OLLAMA_URL;
    }
    
    return config;
};

auto main(int argc, char *argv[]) -> int {
	const Config config = load_config();
    LogVerbosity verbosity = LogVerbosity::DEBUG;  // default verbosity
    CringeBot cringe(config.token, config.ollama, config.openai_key, verbosity);
    return 0;
}
