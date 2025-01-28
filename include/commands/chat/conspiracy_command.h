#ifndef CRINGE_CONSPIRACY_COMMAND_H
#define CRINGE_CONSPIRACY_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand conspiracy_declaration();
void conspiracy_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif