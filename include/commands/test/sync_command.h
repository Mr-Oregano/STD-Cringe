#ifndef CRINGE_SYNC_COMMAND_H
#define CRINGE_SYNC_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand sync_declaration();
void sync_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif