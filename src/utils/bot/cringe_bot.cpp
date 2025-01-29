#include "utils/bot/cringe_bot.h"

// #include <random/random_device.h>
#include <dpp/unicode_emoji.h>
#include <random>
#include <unistd.h>
#include <utils/misc/cringe_environment.h>

#include "commands/chat/challenge_command.h"
#include "commands/chat/chat_command.h"
#include "utils/audio/cringe_audio.h"
#include "utils/database/chat_store.h"
#include "utils/embed/cringe_embed.h"
#include "utils/http/cringe_openai.h"
#include "utils/listeners/command_handler.h"
#include "utils/listeners/cringe_slashcommand.h"

CringeBot::CringeBot(
	const std::string &token,
	const std::string &ollama_url,
	const std::string &openai_key,
	LogVerbosity verbosity)
	: ollama(ollama_url),
	  openai(openai_key),
	  cluster(token, dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_voice_states),
	  logger(verbosity) {

	logger.logStart(); // Show the startup banner

	if (!openai_key.empty()) {
		logger.log(LogLevel::WARN, "OpenAI integration enabled!");
	} else {
		logger.log(LogLevel::INFO, "OpenAI integration disabled - no API key provided");
	}

	// Initialize troll mute map with default guilds enabled
	troll_mute_enabled = {
		{1069835760859107368, true} // Add your guild IDs here
	};

	const std::string system_prompt = get_env("SYSTEM_ROLE");

	cluster.set_websocket_protocol(dpp::ws_etf);
	// cluster.cp({
	// 	dpp::cp_lazy,
	// 	dpp::cache_policy::cpol_balanced,
	// });

	db = std::make_unique<CringeDB>("cringe.db");
	message_store = std::make_unique<MessageStore>(*db);
	embed_store = std::make_unique<EmbedStore>(*db);
	story_store = std::make_unique<StoryStore>(*db);
	conspiracy_store = std::make_unique<ConspiracyStore>(*db);
	queue_store = std::make_unique<QueueStore>(*db);
	challenge_store = std::make_unique<CringeChallengeStore>(*db);
	chat_store = std::make_unique<ChatStore>(*db);
	confession_store = std::make_unique<ConfessionStore>(*db);
	rage_store = std::make_unique<RageStore>(*db);
	rage_state.last_bait = std::chrono::system_clock::now();
	// cringe::CommandHandler command_handler(cluster);

	logger.setDatabase(db.get());

	// Set up message handler
	cluster.on_message_create([this](const dpp::message_create_t &event) {
		if (!event.msg.author.is_bot()) {
			message_store->storeMessage(event.msg);
		}
	});

	cluster.on_message_create([this, system_prompt](const dpp::message_create_t& event) {
		if (event.msg.author.is_bot()) return;

		// Store the last active channel
		dpp::snowflake last_channel = event.msg.channel_id;

		// Check if it's time to send ragebait (roughly every hour)
		auto now = std::chrono::system_clock::now();

		std::cout << fmt::format("Last rage: {}", std::chrono::duration_cast<std::chrono::minutes>(
		now - rage_state.last_bait).count()) << std::endl;

		auto should_rage = std::chrono::duration_cast<std::chrono::minutes>(now - rage_state.last_bait).count() >= 60;

		// Check for replies to our ragebait
		if (event.msg.message_reference.message_id) {
			auto referenced_msg = cluster.message_get_sync(
				event.msg.message_reference.message_id, event.msg.channel_id);

			if (referenced_msg.author.id == cluster.me.id) {
				std::cout << "replied to ragebait" << std::endl;
				// Store the user's reply
				RageInteraction interaction{
					event.msg.id,
					event.msg.author.id,
					event.msg.guild_id,
					event.msg.content,
					"user_reply",
					1.0,
					std::chrono::system_clock::now(),
					referenced_msg.id,  // Store what message this is replying to
					false              // Not a bot message
				};
				rage_store->storeRageInteraction(interaction);
				
				// Get this specific user's conversation thread
				auto conversation_thread = rage_store->getConversationThread(
					event.msg.id,         // Start from current message
					event.msg.author.id   // Get only this user's conversation
				);

				std::string initial = "Create a racially motivated, politically incorrect, and rage inducing comment that will bring about arguments. Your comment should be rage-bait, geared at being as incorrect and insulting as possible.";

				// Build context pairs for Ollama
				std::vector<std::pair<std::string, std::string>> context;
				
				// Find the initial bait and add it as first context pair
				for (size_t i = 0; i < conversation_thread.size(); i++) {
					if (conversation_thread[i].reaction == "initial_bait") {
						// Add a generic "initial" message paired with the bait
						context.push_back({
							initial,                       		// Pretend user message
							conversation_thread[i].content  	// Initial bait as response
						});
						break;
					}
				}

				// Then add the rest of the conversation pairs
				for (size_t i = 0; i < conversation_thread.size() - 1; i++) {
					if (!conversation_thread[i].is_bot_message && i + 1 < conversation_thread.size()) {
						// Add user message and bot's response as a pair
						context.push_back({
							conversation_thread[i].content,      // User's message
							conversation_thread[i + 1].content   // Bot's response
						});
					}
				}

				std::cout << "Context pairs:" << std::endl;
				for (const auto& pair : context) {
					std::cout << "User: " << pair.first << std::endl;
					std::cout << "Bot: " << pair.second << std::endl;
					std::cout << "---" << std::endl;
				}

				cluster.channel_typing(event.msg.channel_id);

				try {
					json response = ollama.chat(
						"troll", 
						event.msg.content,
						"",
						context
					);
					std::string comeback = response["response"];

					dpp::message reply(event.msg.channel_id, comeback);
					reply.set_reference(event.msg.id);
					auto new_message = cluster.message_create_sync(reply);

					// Store bot's response in the chain
					RageInteraction bot_response{
						new_message.id,         // The actual message ID
						cluster.me.id,        // Bot's ID
						event.msg.guild_id,
						comeback,
						"bot_reply",
						1.0,
						std::chrono::system_clock::now(),
						event.msg.id,         // Reference to the user's message
						true                  // Is bot message
					};
					rage_store->storeRageInteraction(bot_response);

				} catch (const std::exception& e) {
					logger.log(LogLevel::ERROR,
						fmt::format("Failed to generate comeback: {}", e.what()));
				}
			}
			return;
		}

		// Initial ragebait
		if (should_rage && !event.msg.message_reference.message_id) {
			// bot will rage
			std::cout << "Bot should rage" << :: std::endl;
			cluster.channel_typing(event.msg.channel_id);
			
			// Get recent messages for context
			std::vector<GuildMessage> messages = message_store->getRecentGuildMessages(event.msg.guild_id, 500);
			std::reverse(messages.begin(), messages.end());
			// Generate ragebait prompt
			// std::string prompt = "Create a racially motivated, politically incorrect, and rage inducing comment that will bring about arguments. Your comment should be rage-bait, geared at being as incorrect and insulting as possible."
			// "\n\nAnalyze the conversation chain below for anything you can start an argument with:\n";

			std::vector<GuildMessage> random_messages;
			GuildMessage random_message;
			for (const auto& msg : messages) {
				if (msg.author_id != cluster.me.id && msg.author_name != cluster.me.username && msg.content.length() > 100) {
					random_messages.push_back(msg);
				}
			}

			// Get a random message if we have any
			if (!random_messages.empty()) {
				// Create a random number generator
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(0, random_messages.size() - 1);

				// Get random index and message
				int random_index = dis(gen);
				random_message = random_messages[random_index];
			}

			// prompt += "\n\nIMPORTANT: Your output should ONLY be the insulting response, with no other descriptive text or thoughts. Your response should be short, only 200-300 characters in length.\n";
			// prompt += "IMPORTANT: If you start an argument based off the message history, you should always @ mention the user (by using `<@user_id>`) in which you are starting the argument with. The first word in your initial response should be the users mention, and you should naturally describe why you are arguing with them (in the rudest way possible).";
			std::string prompt = fmt::format("MAKE FUN OF THIS MESSAGE, AND GENERATE A RAGE-BAIT REPLY TO IT: {}", random_message.content);

			try {
				// Generate ragebait using AI
				json response = ollama.chat("troll", prompt);
				std::string ragebait = response["response"];

				// Send the ragebait as a reply to the random message
				dpp::message msg(last_channel, ragebait);
				msg.set_reference(random_message.id);  // Set as reply to the random message
				auto sent_msg = cluster.message_create_sync(msg);

				
				// Store the initial ragebait
				RageInteraction bait{
					sent_msg.id,			  // The actual message ID from Discord
					cluster.me.id,            // Bot's ID
					event.msg.guild_id,
					ragebait,                 // The content
					"initial_bait",           // Mark as initial bait
					1.0,                      // Base anger score
					std::chrono::system_clock::now(),
					0,                        // No reference for initial bait
					true                      // Is bot message
				};
				rage_store->storeRageInteraction(bait);
				rage_state.last_bait = now;

			} catch (const std::exception& e) {
				logger.log(LogLevel::ERROR, fmt::format("Failed to generate ragebait: {}", e.what()));
			}
		}
	});

	// cluster.on_message_create([this](const dpp::message_create_t& event) {
	// 	if (!event.msg.message_reference.message_id || event.msg.author.is_bot()) {
	// 		return;
	// 	}
	//
	// 	// Set a timeout for the entire operation
	// 	auto timeout = std::chrono::seconds(10);
	// 	auto deadline = std::chrono::steady_clock::now() + timeout;
	//
	// 	try {
	// 		auto promise_ptr = std::make_shared<std::promise<void>>();
	// 		auto operation_future = promise_ptr->get_future();
	//
	// 		cluster.message_get(
	// 			event.msg.message_reference.message_id,
	// 			event.msg.channel_id,
	// 			[this, event, promise_ptr](const dpp::confirmation_callback_t& callback) mutable {
	// 				try {
	// 					if (callback.is_error()) {
	// 						promise_ptr->set_exception(std::make_exception_ptr(
	// 							std::runtime_error("Failed to get referenced message: " + callback.get_error().message)));
	// 						return;
	// 					}
	//
	// 					auto referenced_msg = std::get<dpp::message>(callback.value);
	// 					uint64_t conversation_id;
	//
	// 					if (!referenced_msg.embeds.empty() && extract_conversation_id(referenced_msg, conversation_id)) {
	// 						handle_chat_reply(*this, event, conversation_id);
	// 					}
	//
	// 					promise_ptr->set_value();
	// 				} catch (const std::exception& e) {
	// 					promise_ptr->set_exception(std::current_exception());
	// 				}
	// 			}
	// 		);
	//
	// 		// Wait for operation with timeout
	// 		if (operation_future.wait_until(deadline) == std::future_status::timeout) {
	// 			throw std::runtime_error("Operation timed out");
	// 		}
	// 		operation_future.get();
	//
	// 	} catch (const std::exception& e) {
	// 		logger.log(LogLevel::ERROR, fmt::format("Error processing message reply: {}", e.what()));
	//
	// 		// Send error message to channel
	// 		dpp::message error_msg(event.msg.channel_id,
	// 			"Sorry, I couldn't process your reply. Please try again later.");
	// 		error_msg.set_reference(event.msg.id);
	// 		cluster.message_create(error_msg);
	// 	}
	// });

    cluster.on_form_submit([this](const dpp::form_submit_t& event) {
        if (event.custom_id == "challenge_answer") {
            handle_challenge_interaction(*this, event);
			return;
        }

		if (event.custom_id.starts_with("chat_")) {
			handle_chat_modals(*this, event);
			return;
		}
    });

	cluster.on_message_create([this](const dpp::message_create_t& event) {
	    // Ignore bot messages to prevent loops
	    if (event.msg.author.is_bot()) {
	        return;
	    }
		return;

	    // Check if the message mentions the bot
	    bool is_mentioned = false;
	    for (const auto& mention : event.msg.mentions) {
	        if (mention.first == cluster.me.id) {
	            is_mentioned = true;
	            break;
	        }
	    }

	    if (!is_mentioned) {
	        return;
	    }

	    // Set the typing indicator
	    cluster.channel_typing(event.msg.channel_id);

	    // Remove the mention from the message content
	    std::string content = event.msg.content;
	    size_t mention_pos = content.find("<@" + std::to_string(cluster.me.id) + ">");
	    if (mention_pos != std::string::npos) {
	        content.erase(mention_pos, std::to_string(cluster.me.id).length() + 3);
	    }

	    // Trim whitespace
	    content = std::regex_replace(content, std::regex("^\\s+|\\s+$"), "");

	    if (content.empty()) {
	        content = "Hello!"; // Default prompt if just mentioned
	    }

	    // Ensure the response is less than 2000 characters
	    const std::string sys_prompt = get_env("SYSTEM_ROLE");

		std::cout << content << std::endl;

		try {
		    // Get AI response using the default "cringe" model
		    json response = ollama.oneshot("cringe", content);
		    std::string reply = response["response"];
			if (reply.length() > 1500) {
				reply = reply.substr(0, 1500);
			}
		    logger.log(LogLevel::INFO, fmt::format("AI response: {}", reply));

		    // Create and send reply message
		    dpp::message msg(event.msg.channel_id, reply);
		    msg.set_reference(event.msg.id);
		    cluster.message_create(msg);

		} catch (const std::exception& e) {
		    logger.log(LogLevel::ERROR, fmt::format("Error generating mention response: {}", e.what()));

		    // Send error message
		    dpp::message error_msg(event.msg.channel_id,
		        "Sorry, I couldn't process your message right now. Please try again later.");
		    error_msg.set_reference(event.msg.id);
		    cluster.message_create(error_msg);
		}
	});

	cluster.on_select_click([this](const dpp::select_click_t &event) {
		if (event.custom_id == "challenge_select") {
            handle_challenge_interaction(*this, event);
        }
		if (event.custom_id == "story_select") {
			try {
				dpp::snowflake story_id = std::stoull(event.values[0]);
				auto story = story_store->getStory(story_id);

				if (!story) {
					throw std::runtime_error("Story not found!");
				}

				// Create paginated view of the selected story
				auto embed = CringeEmbed()
						.setTitle(story->title);

				std::vector<dpp::embed> pages;

				// First page: Story overview
				embed.setDescription(fmt::format(
					"**Theme:** {}\n"
					"**Participants:** {}/{}\n"
					"**Current Turn:** <@{}>\n\n"
					"Use the arrows below to navigate through the story parts.",
					story->theme,
					story->participants.size(),
					story->max_participants,
					story->participants[story->current_turn % story->participants.size()]
				));
				pages.push_back(embed.build());

				// Add story part pages
				const size_t PARTS_PER_PAGE = 2;
				for (size_t i = 0; i < story->story_parts.size(); i += PARTS_PER_PAGE) {
					std::string page_content;
					for (size_t j = i; j < std::min(i + PARTS_PER_PAGE, story->story_parts.size()); j++) {
						auto contributor_id = story->participants[j % story->participants.size()];
						page_content += fmt::format("\n\n**Part {}** (by <@{}>):\n{}",
						                            j + 1,
						                            contributor_id,
						                            story->story_parts[j]
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
					cluster,
					*embed_store
				);
			} catch (const std::exception &e) {
				event.reply(dpp::message()
					.set_content(fmt::format("Error: {}", e.what()))
					.set_flags(dpp::m_ephemeral)
				);
			}
		}
	});

	cluster.on_button_click([this](const dpp::button_click_t &event) {
		// if (event.custom_id.starts_with("chat_reply:")) {
		// 	handle_chat_reply(*this, event);
		// 	return;
		// }

		if (event.custom_id.starts_with("chat_")) {
			handle_chat_buttons(*this, event);
			return;
		}

		if (event.custom_id.starts_with("challenge_")) {
            handle_challenge_interaction(*this, event);
			return;
        }

		if (event.custom_id == "prev_page" || event.custom_id == "next_page") {
			// Get embed pages from database
			auto pages_opt = embed_store->getEmbedPages(event.command.msg.id);
			if (!pages_opt) {
				event.reply(dpp::ir_update_message, "This embed has expired.");
				return;
			}

			// Get current page
			size_t current_page = embed_store->getCurrentPage(event.command.msg.id);
			const auto &pages = *pages_opt;

			// Update page based on button clicked
			if (event.custom_id == "prev_page" && current_page > 0) {
				current_page--;
			} else if (event.custom_id == "next_page" && current_page < pages.size() - 1) {
				current_page++;
			}

			// Update database
			embed_store->updateCurrentPage(event.command.msg.id, current_page);

			// Create updated message
			dpp::message updated_msg;
			updated_msg.add_embed(pages[current_page]);
			updated_msg.add_component(CringeEmbed::createNavigationRow(current_page, pages.size()));

			event.reply(dpp::ir_update_message, updated_msg);
		}
	});

	cluster.on_log([this](const dpp::log_t &event) { logger.logEvent(event); });

	cluster.on_slashcommand([this](const dpp::slashcommand_t &event) {
		logger.logSlashCommand(event);
		// command_handler.handle_interaction(event);
		process_slashcommand(event, *this);
	});

	cluster.on_message_delete([this](const dpp::message_delete_t& event) {
	    try {
	        auto channel = cluster.channel_get_sync(event.channel_id);
	        std::string channel_name = channel.name;

	        // First, get the message info from our database before it's gone
	        auto msg_data = message_store->getMessageById(event.id);

	        // If we don't have the message in our database, we can't do much
	        if (!msg_data) {
	            logger.logMessageDelete(channel_name, "unknown", "unknown", "");
	            return;
	        }

	        // Check the audit log - if there's no entry, it was self-deleted
			cluster.guild_auditlog_get(
				event.guild_id, 
				0,
				dpp::aut_message_delete,
				0,
				0,
				1,
				[this, channel_name, event, msg_data](const dpp::confirmation_callback_t& callback) {
					std::string deleter_name;

					// If we got an audit log entry AND it's recent, it was a mod deletion
					if (!callback.is_error() && !std::get<dpp::auditlog>(callback.value).entries.empty()) {
						auto last_entry = std::get<dpp::auditlog>(callback.value).entries.back();
						deleter_name = fmt::format("<@{}>", last_entry.user_id);
					} else {
						// No audit log entry means the user deleted their own message
						deleter_name = msg_data->author_name + " (self)";
					}

					// Log the deletion
					logger.logMessageDelete(
						channel_name,
						msg_data->author_name,
						deleter_name,
						msg_data->content
					);

					// Create and send error embed
					auto embed = CringeEmbed::createError("A message was deleted!")
						.setTitle("Message Deletion Detected")
						.addField("Channel", fmt::format("#{}", channel_name), true)
						.addField("Deleted By", deleter_name, true)
						.addField("Original Author", msg_data->author_name, true)
						.addField("Content", msg_data->content, false)
						.setTimestamp(std::time(nullptr));

					// Send the embed to the channel where deletion occurred
					cluster.message_create(dpp::message(event.channel_id, embed.build()));


	                // Clean up the deleted message from database
	                // message_store->deleteMessage(event.id);
	            }
	        );
	    } catch (const std::exception& e) {
	        logger.log(LogLevel::ERROR, fmt::format("Error in message delete handler: {}", e.what()));
	    }
	});

	cluster.on_message_create([this](const dpp::message_create_t &event) {
		logger.logMessage(event);
		// if (event.msg.author.id == 1) {
		// 	if (rand() % 100 >= 1) {
		// 		cluster.message_add_reaction(event.msg, dpp::unicode_emoji::regional_indicator_f);
		// 		sleep(1);
		// 		cluster.message_add_reaction(event.msg, dpp::unicode_emoji::regional_indicator_a);
		// 		sleep(1);
		// 		cluster.message_add_reaction(event.msg, dpp::unicode_emoji::regional_indicator_g);
		// 		sleep(1);
		// 		cluster.message_add_reaction(event.msg, dpp::unicode_emoji::exclamation);
		// 	}
		// }
	});

	cluster.on_ready([this](const dpp::ready_t &event) {
		if (dpp::run_once<struct register_BOT_commands>()) {
			// register_commands();
			register_slashcommands(*this);
		}
		cluster.set_presence(dpp::presence(dpp::ps_online, dpp::at_custom,
		                                   "active development with <3 by @nulzo"));
	});

	// Send a message to the channel when the message is updated, saying who
	// edited the message and what the original message was/went to
	// cluster.on_message_update([this](const dpp::message_update_t &event) {
	// 	auto channel = event.msg.channel_id;
	// 	if (event.msg.author.is_bot()) {
	// 		return;
	// 	}

	// 	auto embed = CringeEmbed::createWarning("A message was edited in this channel!")
	// 			.setTitle("Message Edit Detected")
	// 			.addField("Author", event.msg.author.get_mention(), true)
	// 			.addField("Edited At", fmt::format("<t:{}:F>", std::time(nullptr)), true)
	// 			.addField("New Content", event.msg.content, false);

	// 	// If we have access to the old message content, add it
	// 	if (!event.msg.content.empty()) {
	// 		embed.addField("Original Content", event.msg.content, false);
	// 	}

	// 	cluster.message_create(dpp::message(channel, embed.build()));
	// });

	cluster.on_voice_state_update([this](
	const dpp::voice_state_update_t &event) {
			// Get the guild ID
			dpp::snowflake guild_id = event.state.guild_id;

			// Check if this update is about our bot
			if (event.state.user_id == cluster.me.id) {
				if (event.state.channel_id) {
					// If someone is trying to move the bot (channel_id changed but
					// we already have a last known channel)
					auto it = last_voice_channels.find(guild_id);
					if (it != last_voice_channels.end() &&
					    it->second != event.state.channel_id) {
						try {
							// Return to the previous channel
							event.from->connect_voice(guild_id, it->second);
							std::cout << "Returning to previous voice channel "
									<< it->second << std::endl;
						} catch (const std::exception &e) {
							std::cerr << "Failed to return to previous channel: "
									<< e.what() << std::endl;
						}
					} else {
						// Bot joined a channel normally through command - update
						// last known channel
						last_voice_channels[guild_id] = event.state.channel_id;
					}
				} else {
					// Bot was disconnected - attempt to rejoin if we know the last
					// channel
					auto it = last_voice_channels.find(guild_id);
					if (it != last_voice_channels.end()) {
						try {
							event.from->connect_voice(guild_id, it->second);
							std::cout << "Attempting to rejoin voice channel "
									<< it->second << std::endl;
						} catch (const std::exception &e) {
							std::cerr
									<< "Failed to rejoin voice channel: " << e.what()
									<< std::endl;
						}
					}
				}
			}
		});

	cluster.on_voice_receive([this](const dpp::voice_receive_t &event) {
		try {
			// Check if voice client exists
			if (!event.voice_client) {
				std::cerr << "Voice client is null\n";
				return;
			}

			// Get the guild ID
			dpp::snowflake guild_id = event.voice_client->server_id;

			// Check if it's our target user
			if (event.user_id != 0) {
				return;
			}

			// Check if troll mute is enabled for this guild
			auto it = troll_mute_enabled.find(guild_id);
			if (it == troll_mute_enabled.end() || !it->second) {
				return;
			}

			// Don't troll the bot itself
			if (event.user_id == cluster.me.id) {
				return;
			}

			// 10% chance to mute them
			if (rand() % 100 >= 1) {
				return;
			}

			// Debug logging
			std::cout << "Attempting to mute user\n";

			// Create guild member object for muting
			dpp::guild_member gm;
			gm.guild_id = guild_id;
			gm.user_id = event.user_id;
			gm.set_mute(true);

			// Use a synchronous call for muting
			cluster.guild_edit_member_sync(gm);
			// cluster.user_set_voice_state_sync(411399698679595008, 1069835760859107368, 0);


			// Create a new thread for unmuting after delay
			std::thread([this, guild_id, user_id = event.user_id]() {
				try {
					// Sleep for 2 seconds
					std::this_thread::sleep_for(std::chrono::seconds(2));

					// Create guild member object for unmuting
					dpp::guild_member gm;
					gm.guild_id = guild_id;
					gm.user_id = user_id;
					gm.set_mute(false);

					// Use a synchronous call for unmuting
					cluster.guild_edit_member_sync(gm);
				} catch (const std::exception &e) {
					std::cerr << "Exception in unmute thread: " << e.what() << "\n";
				}
			}).detach();
		} catch (const std::exception &e) {
			std::cerr << "Exception in voice_receive handler: " << e.what() << "\n";
		}
	});

	// cluster.on_voice_receive([this](const dpp::voice_receive_t& event) {
	// 	try {
	// 		// Check if voice client exists
	// 		if (!event.voice_client) {
	// 			std::cerr << "Voice client is null\n";
	// 			return;
	// 		}

	// 		// Get the guild ID
	// 		dpp::snowflake guild_id = event.voice_client->server_id;

	// 		// Check if it's our target user
	// 		if (event.user_id != 411399698679595008) {
	// 			std::cout << "NOT OUR USER\n";
	// 			return;
	// 		}

	// 		// Debug logging
	// 		std::cout << "Target user detected speaking in guild: " << guild_id << "\n";

	// 		// Check if troll mute is enabled for this guild
	// 		auto it = troll_mute_enabled.find(guild_id);
	// 		if (it == troll_mute_enabled.end() || !it->second) {
	// 			return;
	// 		}

	// 		// First, fetch the guild member to ensure they exist
	// 		cluster.guild_get_member(guild_id, event.user_id,
	// 								 [this, guild_id](const dpp::confirmation_callback_t& callback) {
	// 									 if (callback.is_error()) {
	// 										 std::cerr << "Failed to fetch member: " << callback.get_error().message << "\n";
	// 										 return;
	// 									 }

	// 									 // Now that we confirmed the member exists, disconnect them
	// 									 cluster.guild_member_move(guild_id, 411399698679595008, 0,
	// 															   [](const dpp::confirmation_callback_t& move_callback) {
	// 																   if (move_callback.is_error()) {
	// 																	   std::cerr << "Failed to disconnect user: " << move_callback.get_error().message << "\n";
	// 																   } else {
	// 																	   std::cout << "Successfully disconnected target user from voice channel\n";
	// 																   }
	// 															   }
	// 									 );
	// 								 }
	// 		);

	// 	} catch (const std::exception& e) {
	// 		std::cerr << "Exception in voice_receive handler: " << e.what() << "\n";
	// 	}
	// });

	cluster.start(dpp::st_wait);
}

CringeBot::~CringeBot() = default;
