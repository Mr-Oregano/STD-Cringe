#ifndef CRINGE_MUTE_COMMAND_H
#define CRINGE_MUTE_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand mute_declaration();
void mute_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif
