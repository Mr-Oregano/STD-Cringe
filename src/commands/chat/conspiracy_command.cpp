#include "commands/chat/conspiracy_command.h"
#include <dpp/dpp.h>
#include <thread>
#include "utils/embed/cringe_embed.h"

dpp::slashcommand conspiracy_declaration() {
	return dpp::slashcommand()
			.set_name("conspiracy")
			.set_description("Generate and manage conspiracy theories")
			.add_option(dpp::command_option(dpp::co_sub_command, "generate", "Generate a new conspiracy theory")
								.add_option(dpp::command_option(dpp::co_integer, "messages",
																"Number of recent messages to analyze", false)))
			.add_option(dpp::command_option(dpp::co_sub_command, "view", "View existing conspiracy theories"))
			.add_option(dpp::command_option(dpp::co_sub_command, "evolve", "Make an existing theory more complex")
								.add_option(dpp::command_option(dpp::co_integer, "theory_id",
																"ID of the theory to evolve", true)));
}

void conspiracy_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
	event.thinking(false);

	try {
		dpp::command_interaction cmd_data = event.command.get_command_interaction();
		auto &subcommand = cmd_data.options[0];

		if (subcommand.name == "generate") {
            std::thread([&cringe, event, &subcommand]() {
                try {
                    // Initial loading state
                    auto loading_embed = CringeEmbed::createLoading("Generating Conspiracy Theory", "")
                        .setDescription(
                            "1. Gathering recent messages ⏳\n"
                            "2. Analyzing message patterns ⌛\n"
                            "3. Generating conspiracy theory ⌛\n"
                            "4. Storing in database ⌛\n"
                            "5. Finalizing results ⌛"
                        )
                        .setFooter("Please wait while I uncover the truth...");

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, loading_embed.build()
                    ));

                    // Step 1: Get messages
                    int message_count = 1000; // default
                    if (!subcommand.options.empty()) {
                        message_count = std::get<int64_t>(subcommand.options[0].value);
                    }

                    auto messages = cringe.message_store->getRecentGuildMessages(
                        event.command.guild_id,
                        message_count
                    );
                    
                    if (messages.empty()) {
                        auto error_embed = CringeEmbed::createError(
                            "1. Gathering recent messages ❌\n"
                            "   → Error: No messages found to analyze!\n"
                            "2. Analyzing message patterns ⌛\n"
                            "3. Generating conspiracy theory ⌛\n"
                            "4. Storing in database ⌛\n"
                            "5. Finalizing results ⌛"
                        ).setTitle("Generation Failed");
                        
                        event.edit_original_response(dpp::message(
                            event.command.channel_id, error_embed.build()
                        ));
                        return;
                    }

                    // Update embed after messages gathered
                    auto processing_embed = loading_embed
                        .setDescription(
                            "1. Gathering recent messages ✅\n"
                            "2. Analyzing message patterns ⏳\n"
                            "3. Generating conspiracy theory ⌛\n"
                            "4. Storing in database ⌛\n"
                            "5. Finalizing results ⌛"
                        );

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, processing_embed.build()
                    ));

                    // Step 2: Build AI prompt
                	std::string prompt = fmt::format(
						"Generate a racist, crude, absurd and elaborate conspiracy theory based on these Discord messages. "
						"Create fake connections between users and events. Be creative and humorous. "
						"Format your response exactly like this:\n"
						"TITLE: (a catchy title for the conspiracy)\n"
						"THEORY: (the main conspiracy theory)\n"
						"EVIDENCE: (numbered list of 'evidence' that supports the theory)\n\n"
						"Messages to analyze:\n"
					);

					// Build message context string
					for (const auto& msg : messages) {
						prompt += fmt::format("{}: {}\n", msg.author_name, msg.content);
					}

                    processing_embed.setDescription(
                        "1. Gathering recent messages ✅\n"
                        "2. Analyzing message patterns ✅\n"
                        "3. Generating conspiracy theory ⏳\n"
                        "4. Storing in database ⌛\n"
                        "5. Finalizing results ⌛"
                    );

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, processing_embed.build()
                    ));

                    // Step 3: Get AI response
                    json ollama_response;
                    try {
                        ollama_response = cringe.ollama.chat("cringe", prompt);
                    } catch (const std::exception& e) {
                        auto error_embed = CringeEmbed::createError(
                            fmt::format(
                                "1. Gathering recent messages ✅\n"
                                "2. Analyzing message patterns ✅\n"
                                "3. Generating conspiracy theory ❌\n"
                                "   → Error: {}\n"
                                "4. Storing in database ⌛\n"
                                "5. Finalizing results ⌛",
                                e.what()
                            )
                        ).setTitle("Generation Failed");
                        
                        event.edit_original_response(dpp::message(
                            event.command.channel_id, error_embed.build()
                        ));
                        return;
                    }

                    std::string response = ollama_response["response"];
                    std::string title;
                    
                    // Parse response to extract title
                    size_t title_pos = response.find("TITLE:");
                    size_t theory_pos = response.find("THEORY:");
                    
                    if (title_pos != std::string::npos && theory_pos != std::string::npos) {
                        title = response.substr(title_pos + 6, theory_pos - (title_pos + 6));
                        title = std::regex_replace(title, std::regex("^\\s+|\\s+$"), "");
                    } else {
                        title = "Untitled Conspiracy";
                    }

                    processing_embed.setDescription(
                        "1. Gathering recent messages ✅\n"
                        "2. Analyzing message patterns ✅\n"
                        "3. Generating conspiracy theory ✅\n"
                        "4. Storing in database ⏳\n"
                        "5. Finalizing results ⌛"
                    );

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, processing_embed.build()
                    ));

                    // Step 4: Store in database
                    dpp::snowflake theory_id;
                    try {
                        theory_id = cringe.conspiracy_store->createConspiracy(
                            event.command.guild_id,
                            event.command.usr.id,
                            title,
                            response,
                            {event.command.usr.id},
                            {}
                        );
                    } catch (const std::exception& e) {
                        auto error_embed = CringeEmbed::createError(
                            fmt::format(
                                "1. Gathering recent messages ✅\n"
                                "2. Analyzing message patterns ✅\n"
                                "3. Generating conspiracy theory ✅\n"
                                "4. Storing in database ❌\n"
                                "   → Error: {}\n"
                                "5. Finalizing results ⌛",
                                e.what()
                            )
                        ).setTitle("Generation Failed");
                        
                        event.edit_original_response(dpp::message(
                            event.command.channel_id, error_embed.build()
                        ));
                        return;
                    }

                    // Step 5: Final response
                    auto final_embed = CringeEmbed()
                        .setTitle("🔍 " + title)
                        .setDescription(
                            fmt::format(
                                "**Process Complete:**\n"
                                "1. Gathering recent messages ✅\n"
                                "2. Analyzing message patterns ✅\n"
                                "3. Generating conspiracy theory ✅\n"
                                "4. Storing in database ✅\n"
                                "5. Finalizing results ✅\n\n"
                                "**Generated Theory:**\n{}",
                                response
                            )
                        )
                        .setFooter(fmt::format(
                            "Theory ID: {} | Use /conspiracy evolve {} to make it even more bizarre!",
                            theory_id, theory_id
                        ));

                    event.edit_original_response(dpp::message(
                        event.command.channel_id, final_embed.build()
                    ));

                } catch (const std::exception &e) {
                    event.edit_original_response(dpp::message(
                        event.command.channel_id,
                        CringeEmbed::createError(fmt::format("Error: {}", e.what())).build()
                    ));
                }
            }).detach();
		} else if (subcommand.name == "view") {
			// Get all theories for this guild
			auto theories = cringe.conspiracy_store->getGuildConspiracies(event.command.guild_id);

			if (theories.empty()) {
				event.edit_original_response(dpp::message(
						event.command.channel_id,
						CringeEmbed::createError("No conspiracy theories found for this server!").build()));
				return;
			}

			// Create embed listing all theories
			auto embed = CringeEmbed()
								 .setTitle("🔍 Server Conspiracy Theories")
								 .setDescription("Here are all the active conspiracy theories:");


			for (const auto &theory: theories) {
				// Convert time_point to Unix timestamp
				auto unix_timestamp =
						std::chrono::duration_cast<std::chrono::seconds>(theory.created_at.time_since_epoch()).count();

				// Use the Unix timestamp in the format string

				embed.addField(fmt::format("Theory #{} (Level {})", theory.id, theory.complexity_level),
							   fmt::format("{}\n*Created <t:{}:R>*", theory.title, unix_timestamp), false);
			}

			event.edit_original_response(dpp::message(event.command.channel_id, embed.build()));

		} else if (subcommand.name == "evolve") {
			std::thread([&cringe, event, subcommand]() {
				try {
					dpp::snowflake theory_id = std::get<int64_t>(subcommand.options[0].value);

					// Get the existing theory
					auto theory_opt = cringe.conspiracy_store->getConspiracy(theory_id);
					if (!theory_opt) {
						event.edit_original_response(
								dpp::message(event.command.channel_id,
											 CringeEmbed::createError("Conspiracy theory not found!").build()));
						return;
					}

					auto theory = theory_opt.value();

					// Build AI prompt for evolution
					std::string prompt = fmt::format(
							"Below is an existing conspiracy theory. Make it MORE complex, MORE bizarre, and add NEW "
							"connections. "
							"Keep the same format but expand on the existing theory:\n\n{}\n\n"
							"Make it more elaborate and absurd, but maintain some connection to the original theory.",
							theory.content);

					// Get AI response
					json ollama_response = cringe.ollama.chat("cringe", prompt);
					std::string response = ollama_response["response"];

					// Update the theory
					theory.content = response;
					theory.complexity_level++;
					cringe.conspiracy_store->updateConspiracy(theory);

					// Create response embed
					auto embed = CringeEmbed()
										 .setTitle(fmt::format("🔍 Theory #{} Evolution (Level {})", theory.id,
															   theory.complexity_level))
										 .setDescription(response)
										 .setFooter("The conspiracy grows deeper...");

					event.edit_original_response(dpp::message(event.command.channel_id, embed.build()));

				} catch (const std::exception &e) {
					event.edit_original_response(
							dpp::message(event.command.channel_id,
										 CringeEmbed::createError(fmt::format("Error: {}", e.what())).build()));
				}
			}).detach();
		}
	} catch (const std::exception &e) {
		event.edit_original_response(dpp::message(
				event.command.channel_id, CringeEmbed::createError(fmt::format("Error: {}", e.what())).build()));
	}
}