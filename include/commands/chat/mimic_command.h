#ifndef CRINGE_MIMIC_COMMAND_H
#define CRINGE_MIMIC_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand mimic_declaration();
void mimic_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif