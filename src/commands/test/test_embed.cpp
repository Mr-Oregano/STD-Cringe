#include "commands/test/test_embed.h"
#include "utils/embed/cringe_embed.h"
#include <fmt/format.h>

#include "utils/bot/cringe_bot.h"

dpp::slashcommand test_embed_declaration() {
    return dpp::slashcommand()
        .set_name("test_embed")
        .set_description("Test paginated embeds")
        .add_option(
            dpp::command_option(dpp::co_integer, "pages", "Number of test pages to generate", true)
        );
}

void test_embed_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(true);

    try {
        // Get number of pages from command option
        int num_pages = std::get<int64_t>(event.get_parameter("pages"));
        
        // Create a test embed with multiple fields to force pagination
        auto embed = CringeEmbed::createInfo("Testing Paginated Embeds")
            .setTitle("Pagination Test")
            .setDescription("This is a test of the pagination system.");

        // Add multiple fields with long content to force pagination
        for (int i = 0; i < num_pages; i++) {
            std::string long_content = fmt::format(
                "This is test page {}.\n\n"
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
                "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
                "reprehenderit in voluptate velit esse cillum dolore eu fugiat "
                "nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
                "sunt in culpa qui officia deserunt mollit anim id est laborum.\n\n"
                "Page {} of {} test pages.",
                i + 1, i + 1, num_pages
            );

            embed.addField(
                fmt::format("Test Field {}", i + 1),
                long_content,
                false
            );
        }

        // Build and send paginated embed
        auto pages = embed.buildPaginated();
        embed.sendPaginatedEmbed(pages, event.command.channel_id, cringe.cluster, *cringe.embed_store);

        // Edit the thinking response with a success message
        auto success_embed = CringeEmbed::createSuccess(
            fmt::format("Created a paginated embed with {} pages!", pages.size())
        );
        event.edit_original_response(dpp::message(
            event.command.channel_id, success_embed.build()
        ));

    } catch (const std::exception &e) {
        auto error_embed = CringeEmbed::createError(
            fmt::format("Error creating test embed: {}", e.what())
        );
        event.edit_original_response(dpp::message(
            event.command.channel_id, error_embed.build()
        ));
    }
}