//
// Created by Nolan Gregory on 12/23/24.
//

#ifndef TEST_EMBED_H
#define TEST_EMBED_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand test_embed_declaration();
void test_embed_command(CringeBot &cringe, const dpp::slashcommand_t &event);


#endif //TEST_EMBED_H
