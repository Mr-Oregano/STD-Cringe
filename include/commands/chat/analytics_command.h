#ifndef CRINGE_ANALYTICS_COMMAND_H
#define CRINGE_ANALYTICS_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"
#include <unordered_map>
#include <chrono>

struct UserStats {
    int message_count;
    int voice_minutes;
    int reactions_given;
    int reactions_received;
    std::chrono::system_clock::time_point last_active;
};

struct ServerAnalytics {
    std::unordered_map<dpp::snowflake, UserStats> user_stats;
    std::chrono::system_clock::time_point last_update;
};

dpp::slashcommand analytics_declaration();
void analytics_command(CringeBot &cringe, const dpp::slashcommand_t &event);


#endif