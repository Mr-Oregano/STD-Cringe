#ifndef CRINGE_QUEUE_COMMAND_H
#define CRINGE_QUEUE_COMMAND_H

#include <dpp/dpp.h>
#include <fmt/format.h>
#include "utils/audio/cringe_queue.h"

#include "utils/bot/cringe_bot.h"
#include "utils/misc/cringe_color.h"
#include "utils/misc/cringe_icon.h"

dpp::slashcommand queue_declaration();
void queue_command(CringeBot& cringe, const dpp::slashcommand_t& event);
#endif
