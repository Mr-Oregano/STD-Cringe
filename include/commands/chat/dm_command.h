#ifndef CRINGE_DM_COMMAND_H
#define CRINGE_DM_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand dm_declaration();
void dm_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif