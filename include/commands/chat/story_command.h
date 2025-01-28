#ifndef CRINGE_STORY_COMMAND_H
#define CRINGE_STORY_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"
#include <vector>
#include <chrono>

dpp::slashcommand story_declaration();
void story_command(CringeBot &cringe, const dpp::slashcommand_t &event);

struct StorySession {
    std::string theme;
    std::string title;
    std::vector<dpp::snowflake> participants;
    std::vector<std::string> story_parts;
    size_t current_turn;
    std::chrono::system_clock::time_point last_update;
    bool is_active;
};

#endif