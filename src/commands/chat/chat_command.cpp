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

#include "commands/chat/chat_command.h"
#include <utils/misc/cringe_environment.h>
#include "utils/embed/cringe_embed.h"
#include "utils/http/cringe_ollama.h"

/**
 * @brief Creates a slash command declaration for the chat functionality
 * @param cringe Reference to the CringeBot instance
 * @return dpp::slashcommand The configured slash command
 *
 * Creates a slash command with several subcommands:
 * - new: Start a new chat with an AI model
 * - list: List existing chat conversations
 * - continue: Continue an existing chat conversation
 */
dpp::slashcommand chat_declaration(CringeBot& cringe) {
	dpp::slashcommand command("chat", "Chat and manage conversations with AI models", cringe.cluster.me.id);

	// Create the model choice option
	dpp::command_option model_option = dpp::command_option(dpp::co_string, "model", "The model to interact with", true);
	for (int i = 0; i < std::min(static_cast<int>(cringe.models.size()), 25); i++) {
		model_option.add_choice(dpp::command_option_choice(cringe.models[i], std::string(cringe.models[i])));
	}

	// Chat subcommand (new chat)
	command.add_option(
			dpp::command_option(dpp::co_sub_command, "new", "Start a new chat")
					.add_option(model_option)
					.add_option(dpp::command_option(dpp::co_string, "prompt", "What to ask the model", true)));

	// List chats subcommand
	command.add_option(dpp::command_option(dpp::co_sub_command, "list", "List your chat conversations")
							   .add_option(dpp::command_option(dpp::co_string, "filter", "Filter chats (my/all)", false)
												   .add_choice(dpp::command_option_choice("My chats", "my"))
												   .add_choice(dpp::command_option_choice("All server chats", "all"))));

	// Continue chat subcommand
	command.add_option(dpp::command_option(dpp::co_sub_command, "continue", "Continue a specific chat")
							   .add_option(dpp::command_option(dpp::co_integer, "id", "The conversation ID", true))
							   .add_option(dpp::command_option(dpp::co_string, "message", "Your message", true)));

	return command;
}

/**
 * @brief Handles the chat command execution
 * @param cringe Reference to the CringeBot instance
 * @param event The slash command event containing the command data
 *
 * Processes three subcommands:
 * - new: Creates a new chat conversation with specified model
 * - list: Displays recent chat conversations
 * - continue: Continues an existing conversation
 *
 * @throws std::exception If there's an error processing the command
 */
void chat_command(CringeBot& cringe, const dpp::slashcommand_t& event) {
	const auto& cmd_data = event.command.get_command_interaction();

	// Check if we have any options
	if (cmd_data.options.empty()) {
		event.reply("Invalid command usage!");
		return;
	}

	const auto& subcommand = cmd_data.options[0];

	if (subcommand.name == "new") {
		event.thinking();

		try {
			auto model = std::get<std::string>(subcommand.options[0].value);
			auto prompt = std::get<std::string>(subcommand.options[1].value);

			std::cout << model << " " << prompt << std::endl;

			// Create new conversation
			auto conversation_id =
					cringe.chat_store->createConversation(event.command.usr.id, event.command.guild_id, model);

			// Get AI response
			std::string response;
			if (model == "gpt-4o") {
				json openai_response = cringe.openai.chat(model, prompt);
				response = openai_response["response"];
			} else {
				json ollama_response = cringe.ollama.chat(model, prompt);
				response = ollama_response["response"];
			}

			// Store the message
			ChatMessage msg{0,		conversation_id, event.command.usr.id,
							prompt, response,		 std::chrono::system_clock::now(),
							model};
			cringe.chat_store->storeMessage(msg);

			// Format conversation
			std::string conversation = fmt::format("**You**: {}\n\n**{}**: {}\n\n", prompt, model, response);

			dpp::message chat_msg = create_chat_message(
				"Chat with " + model,
				conversation,
				conversation_id,
				event.command.channel_id,
				model
			);

			event.edit_original_response(chat_msg);


		} catch (const std::exception& e) {
			event.edit_original_response(dpp::message().add_embed(
					CringeEmbed::createError(e.what()).setTitle("Failed to Start Chat").build()));
		}
	} else if (subcommand.name == "list") {
		event.thinking();

		std::string filter = "my";
		if (!subcommand.options.empty()) {
			filter = std::get<std::string>(subcommand.options[0].value);
		}

		std::vector<ChatSummary> chats;
		if (filter == "all") {
			chats = cringe.chat_store->getServerChats(event.command.guild_id);
		} else {
			chats = cringe.chat_store->getUserChats(event.command.usr.id);
		}

		// Sort by last updated
		std::sort(chats.begin(), chats.end(),
				  [](const ChatSummary& a, const ChatSummary& b) { return a.last_updated > b.last_updated; });

		auto embed = CringeEmbed()
							 .setTitle("Recent Conversations")
							 .setDescription(fmt::format(
									 "{} {} active chats\n\n{}", filter == "all" ? "📢" : "👤", chats.size(),
									 chats.empty() ? "No conversations found. Start one with `/chat start`!" : ""));

		// Group chats by recency
		const auto now = std::chrono::system_clock::now();

		for (const auto& chat: chats) {
			const auto hours = std::chrono::duration_cast<std::chrono::hours>(now - chat.last_updated).count();

			std::string title = chat.title.empty() ? fmt::format("Chat with {}", chat.model) : chat.title;

			// Format: Title (messages) • time ago
			std::string field_title =
					fmt::format("{} ({} msg{})", title, chat.message_count, chat.message_count == 1 ? "" : "s");

			std::string field_value =
					fmt::format("ID: {} • <t:{}:R>", chat.id, std::chrono::system_clock::to_time_t(chat.last_updated));

			embed.addField(field_title, field_value, false);

			// Only show last 10 chats to avoid clutter
			if (embed.build().fields.size() >= 10) break;
		}

		embed.setFooter(fmt::format("{} • Continue any chat with /chat continue <id>",
									filter == "all" ? "Server Chats" : "Your Chats"));

		auto pages = embed.buildPaginated();
		if (pages.size() > 1) {
			// If we have multiple pages, use the paginated sender
			embed.sendPaginatedEmbed(pages, event.command.channel_id, cringe.cluster, *cringe.embed_store,
									 event.command.msg.id);
		} else {
			// If only one page, send normally
			event.edit_original_response(dpp::message(event.command.channel_id, embed.build()));
		}
	} else if (subcommand.name == "continue") {
		event.thinking();

		auto conversation_id = std::get<int64_t>(subcommand.options[0].value);
		auto message = std::get<std::string>(subcommand.options[1].value);

		if (!cringe.chat_store->isValidConversation(conversation_id, event.command.usr.id)) {
			event.edit_original_response(
					dpp::message().add_embed(CringeEmbed::createError("You cannot access this conversation!")
													 .setTitle("Access Denied")
													 .build()));
			return;
		}

		auto history = cringe.chat_store->getConversationHistory(conversation_id);
		std::string model = history[0].model;

		std::vector<std::pair<std::string, std::string>> context;
		for (const auto& msg: history) {
			context.push_back({msg.content, msg.response});
		}

		std::string response;
		if (model == "gpt-4o") {
			json openai_response = cringe.openai.chat(model, message, "", context);
			response = openai_response["response"];
		} else {
			json ollama_response = cringe.ollama.chat(model, message, "", context);
			response = ollama_response["response"];
		}

		ChatMessage msg{
				0, conversation_id, event.command.usr.id, message, response, std::chrono::system_clock::now(), model};
		cringe.chat_store->storeMessage(msg);

		history.push_back(msg);

		std::string conversation;
		for (const auto& hist_msg: history) {
			conversation += fmt::format("**You**: {}\n\n**{}**: {}\n\n", hist_msg.content, model, hist_msg.response);
		}

		// Get updated title if needed
		std::string title = generate_chat_summary(cringe, model, history, conversation_id);
		if (title.empty()) {
			title = "Chat with " + model;
		}

		auto chat_embed =
				CringeEmbed()
						.setTitle(title)
						.setDescription(conversation)
						.setFooter(fmt::format("Conversation #{} • Reply to continue chatting", conversation_id));

		dpp::message reply_msg(event.command.channel_id, chat_embed.build());
		event.edit_original_response(reply_msg);
	}
}

/**
 * @brief Generates a summary title for a chat conversation
 * @param cringe Reference to the CringeBot instance
 * @param model The AI model being used
 * @param messages Vector of chat messages in the conversation
 * @param conversation_id The unique identifier of the conversation
 * @return std::string A short summary title for the conversation
 *
 * Generates a 2-6 word summary of the conversation using the AI model.
 * Only generates new summaries at specific message count intervals.
 * This is used as the title on the embeds for chats, as well as for the
 * title within the database.
 */
std::string generate_chat_summary(
	CringeBot& cringe,
	const std::string& model,
	const std::vector<ChatMessage>& messages,
	uint64_t conversation_id) {
	// If there are no stored messages yet, simply return a default chat title
	if (messages.empty()) return "New Chat";

	// Only summarize if we hit a multiple of 5 messages and less than 20
	if (messages.size() > 4 && (messages.size() % 5 != 0 || messages.size() > 20)) return "";

	std::string summary_prompt =
			"Summarize this conversation in a SHORT (between 2 and 6 words), engaging title (max 6 words). Reply ONLY "
			"with the summary and NO other text. Your reply should ALWAYS be less than 6 words:\n\n";
	for (const auto& msg: messages) {
		summary_prompt += fmt::format("User: {}\nAssistant: {}\n\n", msg.content, msg.response);
	}
	summary_prompt +=
			"\n\nIMPORTANT: REPLY ONLY WITH THE SUMMARY THAT SUMMARIZES *ALL* THE MESSAGES. DONT BREAK THEM UP INTO "
			"CHUNKS. IT SHOULD BE A SUMMARY OF THE ENTIRE STRUCTURE SHOWN ABOVE. AN EXAMPLE SUMMARY IS `chat about "
			"dogs`";

	try {
		json response;
		if (model == "gpt-4o") {
			response = cringe.openai.chat(model, summary_prompt);
		} else {
			response = cringe.ollama.chat(model, summary_prompt);
		}

		std::string new_title = response["response"].get<std::string>();

		// Update the title in the database
		cringe.chat_store->updateConversationTitle(conversation_id, new_title);

		return new_title;
	} catch (const std::exception&) {
		return "Chat with " + model;
	}
}

void handle_unauthorized_chat(CringeBot& cringe, const dpp::message_create_t& event, const std::string& model) {
	std::string prompt = fmt::format(
			"Generate a CRAZY, mean, response (1-2 sentences) ROASTING this user. Tell them that they can't "
			"participate in someone else's chat, and ask if they are a freaking idiot or something."
			"Be CREATIVE, FUNNY, humorous and MEAN. Current context: User tried to reply to someone else's chat "
			"conversation.");

	try {
		json response;
		if (model == "gpt-4o") {
			response = cringe.openai.chat(model, prompt);
		} else {
			response = cringe.ollama.chat(model, prompt);
		}

		// Create a proper embed for the unauthorized message
		auto embed = CringeEmbed::createError(response["response"].get<std::string>())
							 .setTitle("Nice Try! 🤭")
							 .setFooter("This isn't your chat conversation!");

		dpp::message reply(event.msg.channel_id, embed.build());
		reply.set_reference(event.msg.id);
		cringe.cluster.message_create(reply);

	} catch (const std::exception&) {
		// Fallback message if AI fails
		auto embed = CringeEmbed::createError("Nice try, but this isn't your chat conversation! 🤭")
							 .setTitle("Access Denied")
							 .setFooter("This isn't your chat conversation");

		dpp::message reply(event.msg.channel_id, embed.build());
		reply.set_reference(event.msg.id);
		cringe.cluster.message_create(reply);
	}
}

/**
 * @brief Generates a summary title for a chat conversation
 * @param cringe Reference to the CringeBot instance
 * @param event The message create event that was a result of replying
 * @param conversation_id The unique identifier of the conversation
 * @return void
 *
 * Replies to a chat reply (i.e. a user replies to a chat message from the
 * bot with the expectation that the bot will issue back a reply).
 */
// void handle_chat_reply(CringeBot& cringe, const dpp::message_create_t& event, uint64_t conversation_id) {
//     cringe.logger.log(
//     	LogLevel::INFO,
//     	fmt::format("Starting chat reply handler for conversation: {}",
//         conversation_id));
//
// 	// Show typing indicator while we process (shows up as `<BOT> is typing...` in Discord)
// 	cringe.cluster.channel_typing(event.msg.channel_id);
//
//     try {
//         cringe.logger.log(LogLevel::INFO, "Fetching conversation history");
//     	// Fetch the relevant conversation history
//         auto history = cringe.chat_store->getConversationHistory(conversation_id);
//         std::string model = history[0].model;
//
//         cringe.logger.log(
//         	LogLevel::INFO,
//         	fmt::format("Retrieved history. Model: {}, User: {}",
//             model, history[0].user_id));
//
//     	// Validate whether a user has the relevant permissions to either reply to a chat
//     	// or whether to dismiss the attempted reply.
//         if (history[0].user_id != event.msg.author.id) {
//             cringe.logger.log(LogLevel::WARN, "Unauthorized chat access attempt");
//             handle_unauthorized_chat(cringe, event, model);
//             return;
//         }
//
//         // Build the chat context
//         std::vector<std::pair<std::string, std::string>> context;
//         for (const auto& msg: history) {
//             context.push_back({msg.content, msg.response});
//         }
//
//         // Get AI response from either Ollama or OpenAI provider (based on model)
//         std::string response;
//         if (model == "gpt-4o") {
//             json openai_response = cringe.openai.chat(model, event.msg.content, "", context);
//             response = openai_response["response"];
//         } else {
//             json ollama_response = cringe.ollama.chat(model, event.msg.content, "", context);
//             response = ollama_response["response"];
//         }
//
//     	// Construct the chat message struct
//         ChatMessage msg{0,
//                         conversation_id,
//                         event.msg.author.id,
//                         event.msg.content,
//                         response,
//                         std::chrono::system_clock::now(),
//                         model};
//     	// Store the reply message to the database
//         cringe.chat_store->storeMessage(msg);
//
//     	// Add to chat history and format conversation
//         history.push_back(msg);
//         std::string conversation;
//         for (const auto& hist_msg: history) {
//             conversation += fmt::format(
//             	"**{}**\n{}\n\n**{}**\n{}\n\n",
//             	event.msg.author.username,
//             	hist_msg.content,
//             	model,
//             	hist_msg.response);
//         }
//
//     	// Get updated title if needed (generate AI summary)
//         std::string title = generate_chat_summary(cringe, model, history, conversation_id);
// 		// If the title is an empty string, create default title
//     	if (title.empty()) {
//             title = cringe.chat_store->getConversationTitle(conversation_id);
//             if (title.empty()) {
//                 title = "Chat with " + model;
//             }
//         }
//
//         // Create chat embed
//         auto chat_embed = CringeEmbed()
//                                     .setTitle(title)
//                                     .setDescription(conversation)
//                                     .setFooter(fmt::format(
//                                     	"Conversation #{} • Reply to continue chatting",
//                                     	conversation_id));
//
//         // Build embeds and get page context (if should be paginated)
//         auto pages = chat_embed.buildPaginated();
//     	// Send paginated embeds or send single embed
//         if (pages.size() > 1) {
//             // If we have multiple pages, use the paginated sender with reply reference
//             chat_embed.sendPaginatedEmbed(
//             	pages,
//             	event.msg.channel_id,
//             	cringe.cluster,
//             	*cringe.embed_store,
//             	event.msg.id
//             );
//         } else {
//             // If only one page, send normally as a single embed
//             dpp::message reply_msg = create_chat_message(title, conversation, conversation_id, event.msg.channel_id);
// 			reply_msg.set_reference(event.msg.id);
// 			cringe.cluster.message_create(reply_msg);
//         }
//     } catch (const std::exception& e) {
//         cringe.logger.log(LogLevel::ERROR, fmt::format("Error in chat reply: {}", e.what()));
//         dpp::message error_msg(event.msg.channel_id, "An error occurred: " + std::string(e.what()));
//         error_msg.set_reference(event.msg.id);
//         cringe.cluster.message_create(error_msg);
//     }
// }

/**
 * @brief Fetches a conversation ID from a chat embed
 * @param message The message that contains the embed
 * @param conversation_id The unique identifier of the conversation
 * @return bool
 *
 * Fetches the conversation ID based off the footer text. A valid chat
 * will have some ID, referenced as `conversation #<id>`. This is extracted
 * and used in the conversation_id addr.
 */
// bool extract_conversation_id(const dpp::message& message, uint64_t& conversation_id) {
// 	// If there is no embed, fail and return false
// 	if (message.embeds.empty()) return false;
//
// 	// Fetch the footer text off the first embed (we can both ensure at this
// 	// point that this 1. isn't a nullptr and 2. there are greater than 0 embeds)
//     const auto& footer_text = message.embeds[0].footer->text;
//
// 	// Get the unique ID off the embed footer
//     const size_t conv_start = footer_text.find("#") + 1;
//     const size_t conv_end = footer_text.find(" •");
// 	// Fail if there isn't a unique conversation ID in the expected format
//     if (conv_start == 0 || conv_end == std::string::npos) {
//         return false;
//     }
//
//     try {
//     	// Store the unique conversation ID to the addr passed in and return true
//         conversation_id = std::stoull(footer_text.substr(conv_start, conv_end - conv_start));
//         return true;
//     } catch (const std::exception& e) {
//     	// Fail and return false
//         std::cout << "Failed to parse conversation ID: " << e.what() << std::endl;
//         return false;
//     }
// }

// void setup_chat_buttons(CringeBot& cringe) {
//     cringe.cluster.on_button_click([&cringe](const dpp::button_click_t& event) {
//         std::string custom_id = event.custom_id;
//
//         if (custom_id.starts_with("chat_regenerate_")) {
//             uint64_t conversation_id = std::stoull(custom_id.substr(15));
//
//             if (!cringe.chat_store->isValidConversation(conversation_id, event.command.usr.id)) {
//                 event.reply(dpp::message("You cannot modify this conversation!").set_flags(dpp::m_ephemeral));
//                 return;
//             }
//
//             event.reply(dpp::ir_deferred_update_message, "");
//
//             try {
//                 auto history = cringe.chat_store->getConversationHistory(conversation_id);
//                 if (history.empty()) {
//                     event.edit_response("No messages found in this conversation!");
//                     return;
//                 }
//
//                 auto last_msg = history.back();
//                 std::string model = last_msg.model;
//
//                 std::vector<std::pair<std::string, std::string>> context;
//                 for (size_t i = 0; i < history.size() - 1; i++) {
//                     context.push_back({history[i].content, history[i].response});
//                 }
//
//                 std::string new_response;
//                 if (model == "gpt-4o") {
//                     json openai_response = cringe.openai.chat(model, last_msg.content, "", context);
//                     new_response = openai_response["response"];
//                 } else {
//                     json ollama_response = cringe.ollama.chat(model, last_msg.content, "", context);
//                     new_response = ollama_response["response"];
//                 }
//
//                 cringe.chat_store->updateMessageResponse(last_msg.id, new_response);
//                 history.back().response = new_response;
//
//                 std::string conversation;
//                 for (const auto& msg : history) {
//                     conversation += fmt::format("**You**: {}\n\n**{}**: {}\n\n",
//                         msg.content, model, msg.response);
//                 }
//
//                 event.edit_response(create_chat_message(
//                     cringe.chat_store->getConversationTitle(conversation_id),
//                     conversation,
//                     conversation_id,
//                     event.command.channel_id
//                 ));
//
//             } catch (const std::exception& e) {
//                 event.edit_response("Failed to regenerate response: " + std::string(e.what()));
//             }
//         }
//         else if (custom_id.starts_with("chat_rename_")) {
//             uint64_t conversation_id = std::stoull(custom_id.substr(12));
//
//             if (!cringe.chat_store->isValidConversation(conversation_id, event.command.usr.id)) {
//                 event.reply(dpp::message("You cannot modify this conversation!").set_flags(dpp::m_ephemeral));
//                 return;
//             }
//
//             dpp::interaction_modal_response modal("chat_rename_modal_" + std::to_string(conversation_id), "Rename Chat");
//             modal.add_component(
//                 dpp::component()
//                     .set_label("New Title")
//                     .set_id("new_title")
//                     .set_type(dpp::cot_text)
//                     .set_placeholder("Enter new chat title")
//                     .set_min_length(1)
//                     .set_max_length(100)
//                     .set_required(true)
//             );
//
//             event.dialog(modal);
//         }
//         else if (custom_id.starts_with("chat_delete_")) {
//             uint64_t conversation_id = std::stoull(custom_id.substr(12));
//
//             if (!cringe.chat_store->isValidConversation(conversation_id, event.command.usr.id)) {
//                 event.reply(dpp::message("You cannot modify this conversation!").set_flags(dpp::m_ephemeral));
//                 return;
//             }
//
//             event.reply(dpp::ir_deferred_update_message, "");
//
//             try {
//                 cringe.chat_store->deleteConversation(conversation_id);
//                 event.edit_response(dpp::message().add_embed(
//                     CringeEmbed::createSuccess("Chat conversation deleted successfully!")
//                         .setTitle("Chat Deleted")
//                         .build()
//                 ));
//             } catch (const std::exception& e) {
//                 event.edit_response("Failed to delete conversation: " + std::string(e.what()));
//             }
//         }
//     });
//
//     cringe.cluster.on_form_submit([&cringe](const dpp::form_submit_t& event) {
//         if (event.custom_id.starts_with("chat_rename_modal_")) {
//             uint64_t conversation_id = std::stoull(event.custom_id.substr(17));
//             std::string new_title = std::get<std::string>(event.components[0].components[0].value);
//
//             event.reply(dpp::ir_deferred_update_message, "");
//
//             try {
//                 cringe.chat_store->updateConversationTitle(conversation_id, new_title);
//
//                 auto history = cringe.chat_store->getConversationHistory(conversation_id);
//                 std::string conversation;
//                 for (const auto& msg : history) {
//                     conversation += fmt::format("**You**: {}\n\n**{}**: {}\n\n",
//                         msg.content, msg.model, msg.response);
//                 }
//
//                 event.edit_response(create_chat_message(
//                     new_title,
//                     conversation,
//                     conversation_id,
//                     event.command.channel_id
//                 ));
//
//             } catch (const std::exception& e) {
//                 event.edit_response("Failed to rename conversation: " + std::string(e.what()));
//             }
//         }
//     });
// }

dpp::message create_chat_message(
	const std::string& title,
	const std::string& conversation,
	uint64_t conversation_id,
	dpp::snowflake channel_id,
	const std::string& model) {

    auto chat_embed = CringeEmbed()
        .setTitle(title)
        .setDescription(conversation)
        .setFooter(fmt::format("Conversation #{} • Chat with {} with /chat!", conversation_id, model));

    dpp::message msg(channel_id, chat_embed.build());

    // Add action row with buttons
    msg.add_component(
        dpp::component().add_component(
            dpp::component()
                .set_label("Reply")
                .set_type(dpp::cot_button)
                .set_style(dpp::cos_primary)
                .set_id(fmt::format("chat_reply_{}", conversation_id))
        ).add_component(
            dpp::component()
                .set_label("Regenerate")
                .set_type(dpp::cot_button)
                .set_style(dpp::cos_secondary)
                .set_id(fmt::format("chat_regenerate_{}", conversation_id))
        ).add_component(
            dpp::component()
                .set_label("Rename")
                .set_type(dpp::cot_button)
                .set_style(dpp::cos_secondary)
                .set_id(fmt::format("chat_rename_{}", conversation_id))
        ).add_component(
            dpp::component()
                .set_label("Delete")
                .set_type(dpp::cot_button)
                .set_style(dpp::cos_danger)
                .set_id(fmt::format("chat_delete_{}", conversation_id))
        )
    );

    return msg;
}

void handle_chat_buttons(CringeBot& cringe, const dpp::button_click_t& event) {
    std::string custom_id = event.custom_id;

	if (custom_id.starts_with("chat_reply_")) {
		std::string cid = custom_id.substr(11);
    	std::cout<< "CID is: " << cid << std::endl;
        uint64_t conversation_id = std::stoull(cid);
        
        if (!cringe.chat_store->isValidConversation(conversation_id, event.command.usr.id)) {
            event.reply(dpp::message("You cannot reply to this conversation!").set_flags(dpp::m_ephemeral));
            return;
        }

        dpp::interaction_modal_response modal("chat_reply_modal_" + std::to_string(conversation_id), "Reply to Chat");
		modal.add_component(
			dpp::component()
				.set_label("Your Message")
				.set_id("reply_content")
				.set_type(dpp::cot_text)
				.set_text_style(dpp::text_paragraph)
				.set_placeholder("Type your message here...")
				.set_min_length(1)
				.set_max_length(2000)
				.set_required(true)
		);

        event.dialog(modal);
    }

    if (custom_id.starts_with("chat_regenerate_")) {
    	std::string cid = custom_id.substr(16);
    	std::cout<< "CID is: " << cid << std::endl;
        uint64_t conversation_id = std::stoull(cid);
        
        if (!cringe.chat_store->isValidConversation(conversation_id, event.command.usr.id)) {
            event.reply(dpp::message("You cannot modify this conversation!").set_flags(dpp::m_ephemeral));
            return;
        }

        event.reply(dpp::ir_deferred_update_message, "");

        try {
            auto history = cringe.chat_store->getConversationHistory(conversation_id);
            if (history.empty()) {
                event.edit_response("No messages found in this conversation!");
                return;
            }

            auto last_msg = history.back();
            std::string model = last_msg.model;

            std::vector<std::pair<std::string, std::string>> context;
            for (size_t i = 0; i < history.size() - 1; i++) {
                context.push_back({history[i].content, history[i].response});
            }

            std::string new_response;
            if (model == "gpt-4o") {
                json openai_response = cringe.openai.chat(model, last_msg.content, "", context);
                new_response = openai_response["response"];
            } else {
                json ollama_response = cringe.ollama.chat(model, last_msg.content, "", context);
                new_response = ollama_response["response"];
            }

        	// Inside the regenerate button handler
        	ChatMessage updated_msg = last_msg;
        	updated_msg.response = new_response;
        	cringe.chat_store->updateMessage(updated_msg);
            history.back().response = new_response;

            std::string conversation;
            for (const auto& msg : history) {
                conversation += fmt::format("**You**: {}\n\n**{}**: {}\n\n", 
                    msg.content, model, msg.response);
            }

            event.edit_response(create_chat_message(
                cringe.chat_store->getConversationTitle(conversation_id),
                conversation,
                conversation_id,
                event.command.channel_id,
                model
            ));

        } catch (const std::exception& e) {
            event.edit_response("Failed to regenerate response: " + std::string(e.what()));
        }
    }
    else if (custom_id.starts_with("chat_rename_")) {
        uint64_t conversation_id = std::stoull(custom_id.substr(12));
        
        if (!cringe.chat_store->isValidConversation(conversation_id, event.command.usr.id)) {
            event.reply(dpp::message("You cannot modify this conversation!").set_flags(dpp::m_ephemeral));
            return;
        }

        dpp::interaction_modal_response modal("chat_rename_modal_" + std::to_string(conversation_id), "Rename Chat");
        modal.add_component(
			dpp::component()
				.set_label("New Title")
				.set_id("new_title")
				.set_type(dpp::cot_text)
				.set_text_style(dpp::text_short)
				.set_placeholder("Enter new chat title")
				.set_min_length(1)
				.set_max_length(100)
				.set_required(true)
		);

        event.dialog(modal);
    }
    else if (custom_id.starts_with("chat_delete_")) {
        uint64_t conversation_id = std::stoull(custom_id.substr(12));
        
        if (!cringe.chat_store->isValidConversation(conversation_id, event.command.usr.id)) {
            event.reply(dpp::message("You cannot modify this conversation!").set_flags(dpp::m_ephemeral));
            return;
        }

        event.reply(dpp::ir_deferred_update_message, "");
        
        try {
            cringe.chat_store->deleteConversation(conversation_id);
            event.edit_response(dpp::message().add_embed(
                CringeEmbed::createSuccess("Chat conversation deleted successfully!")
                    .setTitle("Chat Deleted")
                    .build()
            ));
        } catch (const std::exception& e) {
            event.edit_response("Failed to delete conversation: " + std::string(e.what()));
        }
    }
}

void handle_chat_modals(CringeBot& cringe, const dpp::form_submit_t& event) {

	if (event.custom_id.starts_with("chat_reply_modal_")) {
        uint64_t conversation_id = std::stoull(event.custom_id.substr(17));
        std::string message = std::get<std::string>(event.components[0].components[0].value);
		std::cout << "I AM HERE!" << std::endl;
        event.reply(dpp::ir_deferred_update_message, "");

        try {
            auto history = cringe.chat_store->getConversationHistory(conversation_id);
            std::string model = history[0].model;

            std::vector<std::pair<std::string, std::string>> context;
            for (const auto& msg : history) {
                context.push_back({msg.content, msg.response});
            }

            std::string response;
            if (model == "gpt-4o") {
                json openai_response = cringe.openai.chat(model, message, "", context);
                response = openai_response["response"];
            } else {
                json ollama_response = cringe.ollama.chat(model, message, "", context);
                response = ollama_response["response"];
            }

            ChatMessage msg{
                0, conversation_id, event.command.usr.id, 
                message, response, std::chrono::system_clock::now(), model
            };
            cringe.chat_store->storeMessage(msg);
            history.push_back(msg);

            std::string conversation;
            for (const auto& hist_msg : history) {
                conversation += fmt::format("**You**: {}\n\n**{}**: {}\n\n", 
                    hist_msg.content, model, hist_msg.response);
            }

            event.edit_response(create_chat_message(
                cringe.chat_store->getConversationTitle(conversation_id),
                conversation,
                conversation_id,
                event.command.channel_id,
                model
            ));

        } catch (const std::exception& e) {
            event.edit_response("Failed to send message: " + std::string(e.what()));
        }
    }
    else if (event.custom_id.starts_with("chat_rename_modal_")) {
		try {
			            std::string id_str = event.custom_id.substr(18);  // "chat_rename_modal_" is 17 characters
            std::cout << "id_str: " << id_str << std::endl;
            
            uint64_t conversation_id = std::stoull(id_str);
            std::cout << "conversation_id: " << conversation_id << std::endl;
            
            std::string new_title = std::get<std::string>(event.components[0].components[0].value);
            std::cout << "new_title: " << new_title << std::endl;

			event.reply(dpp::ir_deferred_update_message, "");

            cringe.chat_store->updateConversationTitle(conversation_id, new_title);
            
            auto history = cringe.chat_store->getConversationHistory(conversation_id);
            std::string conversation;
            std::string model = history[0].model;
            
            for (const auto& msg : history) {
                conversation += fmt::format("**You**: {}\n\n**{}**: {}\n\n", 
                    msg.content, model, msg.response);
            }

            event.edit_response(create_chat_message(
                new_title,
                conversation,
                conversation_id,
                event.command.channel_id,
                model
            ));

        } catch (const std::exception& e) {
            event.edit_response("Failed to rename conversation: " + std::string(e.what()));
        }
    }
}