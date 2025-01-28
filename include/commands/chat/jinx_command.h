#ifndef CRINGE_JINX_COMMAND_H
#define CRINGE_JINX_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand jinx_declaration();
void jinx_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif
