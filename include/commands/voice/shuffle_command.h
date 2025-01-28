#ifndef CRINGE_SHUFFLE_COMMAND_H
#define CRINGE_SHUFFLE_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand shuffle_declaration();
void shuffle_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif