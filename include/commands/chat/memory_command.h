#ifndef CRINGE_MEMORY_COMMAND_H
#define CRINGE_MEMORY_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand memory_declaration();
void memory_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif