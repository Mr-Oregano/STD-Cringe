#ifndef CRINGE_MOCK_COMMAND_H
#define CRINGE_MOCK_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand mock_declaration();
void mock_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif