#ifndef CRINGE_CRINGEBOT_H
#define CRINGE_CRINGEBOT_H

#include "utils/audio/cringe_audio.h"
#include "utils/audio/cringe_queue.h"
#include "utils/database/challenge_store.h"
#include "utils/database/chat_store.h"
#include "utils/database/conspiracy_store.h"
#include "utils/database/cringe_database.h"
#include "utils/database/embed_store.h"
#include "utils/database/message_store.h"
#include "utils/database/queue_store.h"
#include "utils/database/story_store.h"
#include "utils/http/cringe_ollama.h"
#include "utils/http/cringe_openai.h"
#include "utils/logger/logger.h"
#include "utils/database/confession_store.h"
#include "utils/database/rage_store.h"

struct JinxState {
    std::chrono::system_clock::time_point end_time;
    bool subtle;
};

struct RageState {
    std::chrono::system_clock::time_point last_bait;
    std::unordered_map<dpp::snowflake, uint64_t> active_feuds;
};


class CringeBot {
private:
	std::string _token;
	std::mutex voice_mutex;
	std::unordered_map<dpp::snowflake, dpp::snowflake> last_voice_channels;
	std::unordered_map<dpp::snowflake, bool> troll_mute_enabled;
    std::unique_ptr<CringeDB> db;

public:
	std::vector<std::string> models = {"cringe", "troll", "phi4", "qwen2.5", "ethan", "klim", "joeman", "biden", "trump", "dolphin3", "gpt-3.5-turbo", "gpt-4o"};
	CringeQueue queue;
	CringeOllama ollama;
	CringeOpenAI openai;
	dpp::cluster cluster;
	CringeLogger logger;
	std::unique_ptr<MessageStore> message_store;
	std::unique_ptr<EmbedStore> embed_store;
	std::shared_ptr<CringeAudioStreamer> audio_streamer;
	std::unique_ptr<StoryStore> story_store;
	std::unique_ptr<ConspiracyStore> conspiracy_store;
	std::shared_ptr<QueueStore> queue_store;
	std::unique_ptr<CringeChallengeStore> challenge_store;
	std::unique_ptr<ChatStore> chat_store;
	std::unique_ptr<ConfessionStore> confession_store;
	std::unordered_map<uint64_t, JinxState> jinxed_users;
	std::unique_ptr<RageStore> rage_store;
	RageState rage_state;
	std::future<void> current_stream;

	explicit CringeBot(const std::string &token, const std::string &ollama_url, 
					const std::string &openai_key, LogVerbosity verbosity = LogVerbosity::DEBUG);	
	~CringeBot();
};

#endif
