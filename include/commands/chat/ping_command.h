#ifndef CRINGE_PING_COMMAND_H
#define CRINGE_PING_COMMAND_H

#include "utils/bot/cringe_bot.h"
#include <dpp/dpp.h>

dpp::slashcommand ping_declaration();
void ping_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif // CRINGE_PING_COMMAND_H