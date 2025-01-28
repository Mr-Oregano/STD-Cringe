#ifndef CRINGE_CONFESSION_COMMAND_H
#define CRINGE_CONFESSION_COMMAND_H

#include <dpp/dpp.h>
#include <fmt/core.h>
#include <unordered_set>
#include "utils/bot/cringe_bot.h"

const std::unordered_set<std::string> ALLOWED_MIME_TYPES = {
	"image/jpeg",
	"image/png",
	"image/gif",
	"image/webp"
};

auto confession_declaration() -> dpp::slashcommand;
void confession_command(CringeBot &cringe, const dpp::slashcommand_t &event);

#endif //CRINGE_CONFESSION_COMMAND_H
