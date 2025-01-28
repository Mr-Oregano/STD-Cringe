#ifndef CRINGE_CHAT_COMMAND_H
#define CRINGE_CHAT_COMMAND_H

#include <dpp/dpp.h>
#include <dpp/unicode_emoji.h>
#include "utils/bot/cringe_bot.h"

dpp::slashcommand chat_declaration(CringeBot &cringe);
void chat_command(CringeBot &cringe, const dpp::slashcommand_t &event);
void handle_chat_reply(CringeBot& cringe, const dpp::message_create_t& event, uint64_t conversation_id);
void handle_unauthorized_chat(CringeBot& cringe, const dpp::message_create_t& event, const std::string& model);
std::string generate_chat_summary(CringeBot& cringe, const std::string& model, const std::vector<ChatMessage>& messages, uint64_t conversation_id);
bool extract_conversation_id(const dpp::message& message, uint64_t& conversation_id);
void handle_chat_reply(CringeBot& cringe, const dpp::message_create_t& event, uint64_t conversation_id);
void handle_chat_buttons(CringeBot& cringe, const dpp::button_click_t& event);
void handle_chat_modals(CringeBot& cringe, const dpp::form_submit_t& event);
dpp::message create_chat_message(const std::string& title, const std::string& conversation, 
                               uint64_t conversation_id, dpp::snowflake channel_id, 
                               const std::string& model);
std::string generate_chat_summary(CringeBot& cringe, const std::string& model, 
                                const std::vector<ChatMessage>& messages, 
                                uint64_t conversation_id);

#endif
