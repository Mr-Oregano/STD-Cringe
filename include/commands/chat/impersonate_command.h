#ifndef CRINGE_IMPERSONATE_COMMAND_H
#define CRINGE_IMPERSONATE_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand impersonate_declaration();
void impersonate_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif