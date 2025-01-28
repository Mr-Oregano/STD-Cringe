#include "commands/chat/story_command.h"
#include "utils/embed/cringe_embed.h"
#include <mutex>
#include <unordered_map>
#include <future>

static std::mutex story_mutex;
static std::unordered_map<dpp::snowflake, StorySession> active_stories;

dpp::slashcommand story_declaration() {
    return dpp::slashcommand()
            .set_name("story")
            .set_description("Create collaborative stories with other users")
            .add_option(
                dpp::command_option(dpp::co_sub_command, "start", "Start a new story")
                .add_option(dpp::command_option(dpp::co_string, "theme", "Theme or starting prompt", true))
                .add_option(dpp::command_option(dpp::co_integer, "participants", "Number of participants (2-5)", true))
            )
            .add_option(
                dpp::command_option(dpp::co_sub_command, "join", "Join an active story")
            )
            .add_option(
                dpp::command_option(dpp::co_sub_command, "continue", "Add to the current story")
                .add_option(dpp::command_option(dpp::co_string, "contribution", "Your addition to the story", true))
                .add_option(dpp::command_option(dpp::co_string, "story_id", "The ID of the story to continue", true))
            )
            .add_option(
                dpp::command_option(dpp::co_sub_command, "list", "List all available stories")
            );
}

void story_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(false);

    try {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();
        auto &subcommand = cmd_data.options[0];

        if (subcommand.name == "start") {
            std::thread([&cringe, event, subcommand]() {
                try {
                    std::string theme = std::get<std::string>(subcommand.options[0].value);
                    int64_t num_participants = std::get<int64_t>(subcommand.options[1].value);

                    if (num_participants < 2 || num_participants > 5) {
                        throw std::runtime_error("Number of participants must be between 2 and 5");
                    }

                    // Generate title using AI
                    auto title_embed = CringeEmbed::createLoading("Generating New Story", "Generating title...")
                            .setDescription("Generated title: [ ]\nGenerated starter: [ ]\n")
                            .setFooter("Step 1/2");
                    event.edit_original_response(dpp::message(event.command.channel_id, title_embed.build()));

                    std::string title_prompt = fmt::format(
                        "Create a creative, emoji-enhanced title for a collaborative story with the theme: {}. "
                        "The title should be short (like a book title [2-6 words]), memorable, and include 1-2 relevant emojis. "
                        "Return ONLY the title, nothing else.",
                        theme
                    );

                    auto title_response = cringe.ollama.chat("cringe", title_prompt);
                    std::string story_title = title_response.at("response").get<std::string>();

                    // Generate initial story part
                    auto story_embed = CringeEmbed::createLoading("Generating New Story", "Generating story starter...")
                            .setDescription("Generated title: [x]\nGenerated starter: [ ]\n")
                            .setFooter("Step 2/2");
                    event.edit_original_response(dpp::message(event.command.channel_id, story_embed.build()));

                    std::string story_prompt = fmt::format(
                        "Create an engaging story starter (2-3 sentences) for a collaborative story with the theme: {}",
                        theme
                    );

                    auto story_response = cringe.ollama.chat("cringe", story_prompt);
                    std::string initial_part = story_response.at("response").get<std::string>();

                    // Create story in database
                    dpp::snowflake story_id = cringe.story_store->createStory(
                        event.command.guild_id,
                        event.command.channel_id,
                        theme,
                        story_title,
                        event.command.usr.id,
                        num_participants,
                        initial_part
                    );

                    // Create success message with story info
                    auto story = cringe.story_store->getStory(story_id);
                    if (!story) {
                        throw std::runtime_error("Failed to create story!");
                    }

                    auto embed = CringeEmbed::createSuccess("Story created!")
                            .setTitle(story->title)
                            .setDescription(fmt::format(
                                "**Theme:** {}\n\n{}",
                                story->theme,
                                story->story_parts[0]
                            ))
                            .addField("Participants",
                                      fmt::format("{}/{}", 1, story->max_participants),
                                      true)
                            .addField("Started by",
                                      event.command.usr.get_mention(),
                                      true)
                            .setFooter("Use /story join to participate!");

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, embed.build()
                    ));
                } catch (const std::exception &e) {
                    event.edit_original_response(dpp::message(
                        event.command.channel_id,
                        CringeEmbed::createError(e.what()).build()
                    ));
                }
            }).detach();
        } else if (subcommand.name == "join") {
            std::thread([&cringe, event]() {
                try {
                    // Get active stories in channel
                    auto stories = cringe.story_store->getActiveStoriesInChannel(event.command.channel_id);
                    if (stories.empty()) {
                        throw std::runtime_error("No active stories to join in this channel!");
                    }

                    // Get the most recent active story
                    const auto &story = stories.back();

                    // Check if user is already a participant
                    if (std::find(story.participants.begin(), story.participants.end(),
                                  event.command.usr.id) != story.participants.end()) {
                        throw std::runtime_error("You are already participating in this story!");
                    }

                    // Check if story is full
                    if (story.participants.size() >= story.max_participants) {
                        throw std::runtime_error("This story already has the maximum number of participants!");
                    }

                    // Add participant to story
                    if (!cringe.story_store->addParticipant(story.story_id, event.command.usr.id)) {
                        throw std::runtime_error("Failed to join the story!");
                    }

                    // Get updated story
                    auto updated_story = cringe.story_store->getStory(story.story_id);
                    if (!updated_story) {
                        throw std::runtime_error("Failed to retrieve updated story!");
                    }

                    // Create success message
                    auto embed = CringeEmbed::createSuccess("Successfully joined the story!")
                            .setTitle(updated_story->title)
                            .setDescription(fmt::format(
                                "**Theme:** {}\n\n{}",
                                updated_story->theme,
                                updated_story->story_parts[0]
                            ))
                            .addField("Participants",
                                      fmt::format("{}/{}",
                                                  updated_story->participants.size(),
                                                  updated_story->max_participants),
                                      true)
                            .addField("Current Turn",
                                      fmt::format("<@{}>",
                                                  updated_story->participants[
                                                      updated_story->current_turn % updated_story->participants.
                                                      size()]),
                                      true);

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, embed.build()
                    ));
                } catch (const std::exception &e) {
                    event.edit_original_response(dpp::message(
                        event.command.channel_id,
                        CringeEmbed::createError(e.what()).build()
                    ));
                }
            }).detach();
        } else if (subcommand.name == "continue") {
            std::thread([&cringe, event, subcommand]() {
                try {
                    std::string contribution = std::get<std::string>(subcommand.options[0].value);
                    dpp::snowflake story_id = std::stoull(std::get<std::string>(subcommand.options[1].value));

                    auto fetching_embed = CringeEmbed::createLoading("Fetching story...",
                                                                     "Processing your contribution...")
                            .setTitle("Fetching story from database...")
                            .setFooter("Please wait...");

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, fetching_embed.build()
                    ));

                    // Get specific story
                    auto story_opt = cringe.story_store->getStory(story_id);
                    if (!story_opt) {
                        throw std::runtime_error("Story not found!");
                    }
                    const auto &story = *story_opt;

                    auto processing_embed = CringeEmbed::createLoading("Loading", "Processing your contribution...")
                            .setTitle(story.title)
                            .setFooter("Please wait while I review your addition!");

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, processing_embed.build()
                    ));

                    // Check if user is a participant
                    if (std::find(story.participants.begin(), story.participants.end(),
                                  event.command.usr.id) == story.participants.end()) {
                        throw std::runtime_error("You are not a participant in this story!");
                    }

                    // Check if it's user's turn
                    size_t expected_author_index = story.current_turn % story.participants.size();
                    if (story.participants[expected_author_index] != event.command.usr.id) {
                        throw std::runtime_error(fmt::format(
                            "It's not your turn! Current turn: <@{}>",
                            story.participants[expected_author_index]
                        ));
                    }

                    // Validate contribution
                    auto validating_embed = CringeEmbed::createLoading("Loading", "Validating your contribution...")
                            .setTitle("Validating contribution...")
                            .setDescription("Checking if your addition fits the story's theme and flow.")
                            .setFooter("Please wait...");

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, validating_embed.build()
                    ));

                    std::string validation_prompt = fmt::format(
                        "You are validating a story contribution. The story so far:\n\n{}\n\n"
                        "New contribution: {}\n\n"
                        "Evaluate if this contribution:\n"
                        "1. Continues the story logically\n"
                        "2. Maintains the theme\n"
                        "3. Is appropriate for the story\n\n"
                        "Respond with only VALID or INVALID followed by a brief explanation.",
                        fmt::join(story.story_parts, " "),
                        contribution
                    );

                    auto validation_response = cringe.ollama.chat("cringe", validation_prompt);
                    std::string validation = validation_response.at("response").get<std::string>();

                    if (validation.find("INVALID") != std::string::npos) {
                        throw std::runtime_error(validation);
                    }

                    // Enhance contribution
                    auto expanding_embed = CringeEmbed::createLoading("Loading", "Enhancing your contribution...")
                            .setTitle("Enhancing contribution...")
                            .setDescription("Your addition fits perfectly! Now enhancing it with more detail...")
                            .setFooter("Please wait...");

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, expanding_embed.build()
                    ));

                    std::string prompt = fmt::format(
                        "You are helping to write a collaborative story. The story so far is:\n\n{}\n\n"
                        "A user has added this new part:\n\n{}\n\n"
                        "Please improve this addition to make it flow seamlessly with the previous parts. "
                        "Maintain the same tone and style, but enhance the connection to previous events "
                        "and ensure consistent narrative flow. Keep the core ideas but make it feel natural. "
                        "Return ONLY the improved version of the new addition, nothing else.",
                        fmt::join(story.story_parts, "\n\n"),
                        contribution
                    );

                    auto expanded_response = cringe.ollama.chat("cringe", prompt);
                    std::string expanded_contribution = expanded_response.at("response").get<std::string>();

                    // Add contribution to database
                    if (!cringe.story_store->
                        addStoryPart(story.story_id, event.command.usr.id, expanded_contribution)) {
                        throw std::runtime_error("Failed to add your contribution to the story!");
                    }

                    // Get updated story
                    auto updated_story = cringe.story_store->getStory(story.story_id);
                    if (!updated_story) {
                        throw std::runtime_error("Failed to retrieve updated story!");
                    }

                    // Create paginated view
                    auto embed = CringeEmbed()
                            .setTitle(updated_story->title);

                    std::vector<dpp::embed> pages;

                    // First page: Story overview
                    embed.setDescription(fmt::format(
                        "**Theme:** {}\n"
                        "**Participants:** {}/{}\n"
                        "**Current Turn:** <@{}>\n\n"
                        "Use the arrows below to navigate through the story parts.",
                        updated_story->theme,
                        updated_story->participants.size(),
                        updated_story->max_participants,
                        updated_story->participants[updated_story->current_turn % updated_story->participants.size()]
                    ));
                    pages.push_back(embed.build());

                    // Add story part pages
                    const size_t PARTS_PER_PAGE = 2;
                    for (size_t i = 0; i < updated_story->story_parts.size(); i += PARTS_PER_PAGE) {
                        std::string page_content;
                        for (size_t j = i; j < std::min(i + PARTS_PER_PAGE, updated_story->story_parts.size()); j++) {
                            auto contributor_id = updated_story->participants[j % updated_story->participants.size()];
                            page_content += fmt::format("\n\n**Part {}** (by <@{}>):\n{}",
                                                        j + 1,
                                                        contributor_id,
                                                        updated_story->story_parts[j]
                            );
                        }

                        auto page_embed = embed;
                        page_embed.setDescription(page_content);
                        pages.push_back(page_embed.build());
                    }

                    // Send paginated embed
                    embed.sendPaginatedEmbed(
                        pages,
                        event.command.channel_id,
                        cringe.cluster,
                        *cringe.embed_store
                    );

                    // Update original response
                    event.edit_original_response(dpp::message(
                        event.command.channel_id,
                        CringeEmbed::createSuccess("Your contribution has been added to the story!").build()
                    ));
                } catch (const std::exception &e) {
                    event.edit_original_response(dpp::message(
                        event.command.channel_id,
                        CringeEmbed::createError(e.what()).build()
                    ));
                }
            }).detach();
        } else if (subcommand.name == "view") {
            std::thread([&cringe, event]() {
                try {
                    std::lock_guard<std::mutex> lock(story_mutex);
                    auto it = active_stories.find(event.command.channel_id);
                    if (it == active_stories.end()) {
                        throw std::runtime_error("No active story in this channel!");
                    }

                    const auto &session = it->second;

                    // Get current user asynchronously
                    cringe.cluster.user_get(
                        session.participants[session.current_turn % session.participants.size()],
                        [event, &session, &cringe](const dpp::confirmation_callback_t &cb) {
                            if (!cb.is_error()) {
                                auto current_user = std::get<dpp::user_identified>(cb.value);

                                // Create base embed
                                auto embed = CringeEmbed()
                                        .setTitle(fmt::format("{}", session.title));

                                // First page: Story overview
                                embed.setDescription(fmt::format(
                                    "**Theme:** {}\n"
                                    "**Participants:** {}\n"
                                    "**Current Turn:** {}\n\n"
                                    "Use the arrows below to navigate through the story parts.",
                                    session.theme,
                                    session.participants.size(),
                                    current_user.get_mention()
                                ));

                                // Build pages
                                std::vector<dpp::embed> pages = {embed.build()};

                                // Group story parts (2-3 per page)
                                const size_t PARTS_PER_PAGE = 2;
                                for (size_t i = 0; i < session.story_parts.size(); i += PARTS_PER_PAGE) {
                                    std::string page_content;
                                    for (size_t j = i; j < std::min(i + PARTS_PER_PAGE, session.story_parts.size()); j
                                         ++) {
                                        auto contributor_id = session.participants[j % session.participants.size()];
                                        page_content += fmt::format("\n\n**Part {}** (by <@{}>):\n{}",
                                                                    j + 1,
                                                                    contributor_id,
                                                                    session.story_parts[j]
                                        );
                                    }

                                    auto page_embed = embed;
                                    page_embed.setDescription(page_content);
                                    pages.push_back(page_embed.build());
                                }

                                // Send paginated embed
                                embed.sendPaginatedEmbed(
                                    pages,
                                    event.command.channel_id,
                                    cringe.cluster,
                                    *cringe.embed_store
                                );

                                // Update original response
                                event.edit_original_response(dpp::message(
                                    event.command.channel_id,
                                    CringeEmbed::createSuccess("Story retrieved successfully!").build()
                                ));
                            }
                        }
                    );
                } catch (const std::exception &e) {
                    event.edit_original_response(dpp::message(
                        event.command.channel_id,
                        CringeEmbed::createError(e.what()).build()
                    ));
                }
            }).detach();
        } else if (subcommand.name == "list") {
            std::thread([&cringe, event]() {
                try {
                    auto stories = cringe.story_store->getActiveStoriesInChannel(event.command.channel_id);
                    if (stories.empty()) {
                        throw std::runtime_error("No active stories in this channel!");
                    }

                    // Create embed with story list
                    auto embed = CringeEmbed()
                            .setTitle("Active Stories in Channel")
                            .setDescription("Select a story to view its details:");

                    // Create select menu
                    dpp::component select_menu;
                    select_menu.set_type(dpp::cot_selectmenu)
                            .set_placeholder("Select a story to view")
                            .set_id("story_select");

                    // Add each story to both embed and select menu
                    for (const auto &story: stories) {
                        embed.addField(
                            fmt::format("📖 {}", story.title),
                            fmt::format(
                                "**ID:** {}\n**Theme:** {}\n**Participants:** {}/{}\n**Current Turn:** <@{}>",
                                story.story_id,
                                story.theme,
                                story.participants.size(),
                                story.max_participants,
                                story.participants[story.current_turn % story.participants.size()]
                            ),
                            false
                        );

                        dpp::select_option option;
                        option.set_label(story.title)
                                .set_value(std::to_string(story.story_id))
                                .set_description(fmt::format("Theme: {}", story.theme));
                        select_menu.add_select_option(option);
                    }

                    // Create message with both embed and select menu
                    dpp::message msg(event.command.channel_id, "");
                    msg.add_embed(embed.build());
                    msg.add_component(dpp::component().add_component(select_menu));

                    event.edit_original_response(msg);
                } catch (const std::exception &e) {
                    event.edit_original_response(dpp::message(
                        event.command.channel_id,
                        CringeEmbed::createError(e.what()).build()
                    ));
                }
            }).detach();
        }
    } catch (const std::exception &e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            fmt::format("Error starting command: {}", e.what())
        ));
    }
}
