#ifndef CRINGE_SAY_COMMAND_H
#define CRINGE_SAY_COMMAND_H

#include <dpp/dpp.h>
#include "utils/bot/cringe_bot.h"

// Enum for supported languages
enum class TTSLanguage {
    EN, // English
    ES, // Spanish
    FR, // French
    DE, // German
    IT, // Italian
    JA, // Japanese
    KO, // Korean
    RU, // Russian
    ZH  // Chinese
};

dpp::slashcommand say_declaration();
void say_command(CringeBot& cringe, const dpp::slashcommand_t& event);

#endif