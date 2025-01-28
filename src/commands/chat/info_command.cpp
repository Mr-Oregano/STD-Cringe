/*
 * MIT License
 *
 * Copyright (c) 2023 @nulzo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "fmt/format.h"
#include "commands/chat/info_command.h"
#include "utils/misc/cringe_color.h"
#include "utils/embed/cringe_embed.h"
#include "utils/misc/cringe_icon.h"

dpp::slashcommand info_declaration() {
    return dpp::slashcommand().set_name("info").set_description(
        "General information about the bot");
}

void info_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    auto embed = CringeEmbed::createInfo("Probably the **most based** discord bot...")
        .setTitle("std::cringe bot")
        .setThumbnail(CringeIcon::InfoIcon);

    cringe.cluster.user_get(933796468731568191, [&embed](const dpp::confirmation_callback_t& callback) {
        if (!callback.is_error()) {
            auto user = std::get<dpp::user_identified>(callback.value);
            embed.addField("Creator", user.get_mention(), true);
        }
    });

    cringe.cluster.user_get(1186860332845629511, [&embed](const dpp::confirmation_callback_t& callback) {
        if (!callback.is_error()) {
            auto user = std::get<dpp::user_identified>(callback.value);
            embed.addField("Created", 
                fmt::format("<t:{}:D>", std::to_string((int)user.get_creation_time())),
                true);
        }
    });

    embed.addField("Active In",
        std::to_string(dpp::get_guild_cache()->count()) + " servers",
        true)
        .setFooter("v0.0.1");

    event.reply(dpp::message(event.command.channel_id, embed.build()));
}