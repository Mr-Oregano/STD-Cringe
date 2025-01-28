#include "commands/chat/impersonate_command.h"
#include <sstream>
#include <thread>
#include "utils/embed/cringe_embed.h"

dpp::slashcommand impersonate_declaration() {
	return dpp::slashcommand()
			.set_name("impersonate")
			.set_description("Temporarily become another user")
			.add_option(dpp::command_option(dpp::co_user, "target", "User to impersonate", true))
			.add_option(dpp::command_option(dpp::co_integer, "messages", "Number of messages to send (max 5)", true))
			.add_option(dpp::command_option(dpp::co_integer, "delay", "Delay between messages in seconds", false));
}

void impersonate_command(CringeBot& cringe, const dpp::slashcommand_t& event) {
	event.thinking(true);

	try {
		auto target_user = std::get<dpp::snowflake>(event.get_parameter("target"));
		auto message_count = std::min(std::get<int64_t>(event.get_parameter("messages")), static_cast<int64_t>(5));
		auto delay = event.get_parameter("delay").index() ? std::get<int64_t>(event.get_parameter("delay")) : 2;
		delay = std::min(delay, static_cast<int64_t>(10));

		// Create a shared pointer for the event to use in callbacks
		auto shared_event = std::make_shared<dpp::slashcommand_t>(event);

		// Get guild member info
		auto member = cringe.cluster.guild_get_member_sync(event.command.guild_id, target_user);

		const dpp::guild_member me = cringe.cluster.guild_get_member_sync(event.command.guild_id, cringe.cluster.me.id);

		try {
			auto loading_embed = CringeEmbed::createLoading("Preparing Impersonation", "")
										 .setDescription(
												 "1. Gathering user data ⌛\n"
												 "2. Analyzing patterns ⌛\n"
												 "3. Preparing disguise ⌛\n"
												 "4. Beginning impersonation ⌛");

			shared_event->edit_original_response(dpp::message(shared_event->command.channel_id, loading_embed.build()));

			// Get user's messages
			auto messages =
					cringe.message_store->getMostRecentMessages(shared_event->command.guild_id, target_user, 100);

			if (messages.empty()) {
				throw std::runtime_error("No messages found for this user!");
			}

			// Store original bot nickname
			auto original_nickname = cringe.cluster.me.username;
			auto og_icon = cringe.cluster.me.avatar;

			dpp::guild_member gm;
			gm.guild_id = event.command.guild_id;
			gm.user_id = me.user_id;
			gm.set_nickname(member.get_nickname());
			gm.avatar = member.avatar;

			cringe.cluster.guild_edit_member_sync(gm);

			// Build AI prompt for message generation
			std::string ai_prompt = fmt::format(
					"You are impersonating a Discord user. Generate {} different messages that match their "
					"style.\n\n"
					"Their recent messages:\n{}\n\n"
					"Rules:\n"
					"1. Each message should be on a new line\n"
					"2. Match their writing style, emojis, and typical topics\n"
					"3. Keep messages a similar length to their usual messages\n"
					"4. Make messages feel natural and conversational\n"
					"5. Don't explain or add commentary, just output the messages",
					message_count, fmt::join(messages, "\n"));

			// Update loading status
			loading_embed.setDescription(
					"1. Gathering user data ✅\n"
					"2. Analyzing patterns ✅\n"
					"3. Preparing disguise ✅\n"
					"4. Beginning impersonation ⌛");
			shared_event->edit_original_response(dpp::message(shared_event->command.channel_id, loading_embed.build()));

			// Get AI response
			json response = cringe.ollama.chat("gpt-4", ai_prompt);
			std::string generated = response["response"];
			std::vector<std::string> generated_messages;

			// Split response into individual messages
			std::istringstream iss(generated);
			std::string line;
			while (std::getline(iss, line) && generated_messages.size() < message_count) {
				if (!line.empty()) {
					generated_messages.push_back(line);
				}
			}

			// Send messages with delay
			for (const auto& msg: generated_messages) {
				cringe.cluster.message_create(dpp::message(shared_event->command.channel_id, msg));
				std::this_thread::sleep_for(std::chrono::seconds(delay));
			}

			// Reset nickname
			dpp::guild_member ngm;
			ngm.avatar = og_icon;
			ngm.set_nickname(original_nickname);
			cringe.cluster.guild_edit_member_sync(gm);

			// Final confirmation
			auto success_embed = CringeEmbed::createSuccess(fmt::format(
					"Successfully impersonated {} {} times!", member.get_nickname(), generated_messages.size()));
			shared_event->edit_original_response(dpp::message(shared_event->command.channel_id, success_embed.build()));

		} catch (const std::exception& e) {
			auto error_embed = CringeEmbed::createError(e.what()).setTitle("Impersonation Failed");
			shared_event->edit_original_response(dpp::message(shared_event->command.channel_id, error_embed.build()));
		}

	} catch (const std::exception& e) {
		event.edit_original_response(
				dpp::message(event.command.channel_id, CringeEmbed::createError(e.what()).build()));
    }
}