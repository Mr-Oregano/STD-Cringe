#ifndef TEST_COMMAND_H
#define TEST_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand test_declaration();
void test_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif // TEST_COMMAND_H