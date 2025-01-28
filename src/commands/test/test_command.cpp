#include "commands/test/test_command.h"

#include <dpp/unicode_emoji.h>

#include <fmt/format.h>
#include "utils/embed/cringe_embed.h"

dpp::slashcommand test_declaration() {
    return dpp::slashcommand()
        .set_name("test")
        .set_description("Test various bot features")
        .add_option(
            dpp::command_option(dpp::co_sub_command, "embed", "Test paginated embeds")
            .add_option(dpp::command_option(dpp::co_integer, "pages", "Number of test pages to generate", true))
        )
        .add_option(
            dpp::command_option(dpp::co_sub_command, "emojis", "Test emoji display in embeds")
        );
}

void test_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking();

    try {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();
        auto &subcommand = cmd_data.options[0];
    	auto spinner_emoji_id = cringe.cluster.application_emoji_get_sync(1321309584274620446);
    	auto success_emoji_id = cringe.cluster.application_emoji_get_sync(1321702680103751680);
    	auto error_emoji_id = cringe.cluster.application_emoji_get_sync(1321703126272839690);

        if (subcommand.name == "emojis") {
            auto embed = CringeEmbed::createInfo("Emoji Test")
                .setTitle(fmt::format("Emoji Test {}", dpp::unicode_emoji::small_red_triangle))
                .setDescription(fmt::format(
                    "Testing various emoji placements:\n\n"
                    "**CringeAI Emojis**\n"
                    "{} Spinner: `{}`\n"
                    "{} Success: `{}`\n"
                    "{} Error: `{}`\n\n",
                    spinner_emoji_id.get_mention(),
                    spinner_emoji_id.get_mention(),
                    success_emoji_id.get_mention(),
                    success_emoji_id.get_mention(),
                    error_emoji_id.get_mention(),
                    error_emoji_id.get_mention()
                ))
                .addField(
                    fmt::format("Field with Emoji {}", dpp::unicode_emoji::bookmark_tabs),
                    fmt::format("Value with emoji {}", dpp::unicode_emoji::memo),
                    true
                )
                .setFooter(fmt::format("Footer with emoji {}", dpp::unicode_emoji::information_source));

            event.edit_original_response(dpp::message(
                event.command.channel_id,
                embed.build()
            ));
        }

    } catch (const std::exception &e) {
        auto error_embed = CringeEmbed::createError(
            fmt::format("Error testing emojis: {}", e.what())
        );
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            error_embed.build()
        ));
    }
}