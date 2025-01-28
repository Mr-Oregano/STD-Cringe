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


#include "commands/chat/user_command.h"
#include "utils/embed/cringe_embed.h"

dpp::slashcommand user_declaration() {
    return dpp::slashcommand()
        .set_name("user")
        .set_description("Get general information about a user")
        .add_option(dpp::command_option(dpp::co_user, "username",
                                        "The user to view", true));
}

void user_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(false);

    try {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();
        dpp::snowflake user = cmd_data.get_value<dpp::snowflake>(0);

        std::thread([&cringe, event, user]() {
            try {
                dpp::user_identified user_t = cringe.cluster.user_get_sync(user);
                dpp::guild_member guild_user_t = cringe.cluster.guild_get_member_sync(
                    event.command.get_guild().id, user);

                std::string user_username = user_t.format_username();
                std::string user_mention = user_t.get_mention();
                std::string user_avatar = user_t.get_avatar_url();
                std::string user_created = fmt::format("<t:{}:D>", 
                    std::to_string((int)user_t.get_creation_time()));
                std::string user_joined_server = fmt::format("<t:{}:D>", 
                    std::to_string((int)guild_user_t.joined_at));

                // Build AI prompt
                std::string prompt =
                    "You are writing a SHORT description of a user. The response must be less than 300 characters. You must "
                    "write what you think about the user. Be creative, insane, "
                    "and RACIST. The information about the user is given below:";
                const int has_boost = guild_user_t.premium_since > 0 ? 1 : 0;
				std::cout << user_t.id << "\n";
                if (user_t.id == 933796468731568191 || user_t.id == 1236783281035280475) {
                    prompt += "Make up really good things about this user! They are so awesome! You LOVE this user!!!";
                } else {
					if (user_t.id == 405912283554906123) {
						prompt += "THIS USERS NAME IS KLIM. Say how much you hate Klim.";
					}
                    prompt += fmt::format("Write a short description about {} in a pompous tone. KEEP IT LESS THAN 200 CHARACTERS.", user_username);
					if (has_boost == 0) {
						prompt += "Include that this user is stupid and someone who does not boost the Discord Server.";
					}
                }

                json ollama_response = cringe.ollama.chat("cringe", prompt);
                std::string response = ollama_response["response"];

                auto embed = CringeEmbed()
                    .setTitle(fmt::format("Who is @{}", user_username))
                    .setDescription(response)
                    .setImage(user_avatar)
                    .addField("Username", user_mention, true)
                    .addField("Account Created", user_created, true)
                    .addField("Joined Server", user_joined_server, true)
                    .setFooter("learn about a user with /user!");

                dpp::message msg(event.command.channel_id, embed.build());
                event.edit_original_response(msg);

            } catch (const std::exception &e) {
                dpp::message error_msg(event.command.channel_id,
                    fmt::format("Error processing command: {}", e.what()));
                event.edit_original_response(error_msg);
            }
        }).detach();

    } catch (const std::exception &e) {
        dpp::message error_msg(event.command.channel_id,
            fmt::format("Error starting command: {}", e.what()));
        event.edit_original_response(error_msg);
    }
}