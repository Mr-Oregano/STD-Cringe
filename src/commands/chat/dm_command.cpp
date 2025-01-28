#include "commands/chat/dm_command.h"
#include "utils/embed/cringe_embed.h"
#include <thread>
#include <functional>

dpp::slashcommand dm_declaration() {
    return dpp::slashcommand()
        .set_name("dm")
        .set_description("DM a user multiple times")
        .add_option(dpp::command_option(dpp::co_user, "user", "User to DM", true))
        .add_option(dpp::command_option(dpp::co_integer, "count", "Number of DMs to send (max 50)", true))
        .add_option(dpp::command_option(dpp::co_string, "message", "Message to send", true));
}

void dm_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(false);

    try {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();
        auto target_user = cmd_data.get_value<dpp::snowflake>(0);
        auto count = std::min(cmd_data.get_value<int64_t>(1), static_cast<int64_t>(50));
        auto message = cmd_data.get_value<std::string>(2);

        auto thread_data = std::make_shared<dpp::slashcommand_t>(event);
        auto thread_cringe = &cringe;

        std::thread([thread_data, thread_cringe, target_user, count, message]() {
            try {
                auto loading_embed = CringeEmbed::createLoading("DM Spam Operation", "")
                    .setDescription(
                        fmt::format(
                            "1. Verifying user access ⏳\n"
                            "2. Opening DM channel ⌛\n"
                            "3. Sending messages (0/{}) ⌛\n"
                            "4. Finalizing results ⌛",
                            count
                        )
                    )
                    .setFooter("Please wait while I spam their DMs...");

                thread_data->edit_original_response(dpp::message(
                    thread_data->command.channel_id, loading_embed.build()
                ));

                thread_cringe->cluster.user_get(target_user, [thread_data, thread_cringe, count, message](
                    const dpp::confirmation_callback_t& user_cb) {
                    if (user_cb.is_error()) {
                        auto error_embed = CringeEmbed::createError(
                            "1. Verifying user access ❌\n"
                            "   → Error: Could not find user!\n"
                            "2. Opening DM channel ⌛\n"
                            "3. Sending messages (0/0) ⌛\n"
                            "4. Finalizing results ⌛"
                        ).setTitle("Operation Failed");

                        thread_data->edit_original_response(dpp::message(
                            thread_data->command.channel_id, error_embed.build()
                        ));
                        return;
                    }

                    auto user = std::get<dpp::user_identified>(user_cb.value);

                    thread_cringe->cluster.create_dm_channel(user.id, [thread_data, thread_cringe, user, count, message](
                        const dpp::confirmation_callback_t& dm_cb) {
                        if (dm_cb.is_error()) {
                            auto error_embed = CringeEmbed::createError(
                                "1. Verifying user access ✅\n"
                                "2. Opening DM channel ❌\n"
                                "   → Error: Could not open DM channel!\n"
                                "3. Sending messages (0/0) ⌛\n"
                                "4. Finalizing results ⌛"
                            ).setTitle("Operation Failed");

                            thread_data->edit_original_response(dpp::message(
                                thread_data->command.channel_id, error_embed.build()
                            ));
                            return;
                        }

                        auto dm_channel = std::get<dpp::channel>(dm_cb.value);
                        auto messages_sent = std::make_shared<size_t>(0);
                        auto send_next_message = std::make_shared<std::function<void()>>();

                        *send_next_message = [thread_data, thread_cringe, messages_sent, count, message, dm_channel, user, send_next_message]() {
                            if (*messages_sent >= count) {
                                auto final_embed = CringeEmbed()
                                    .setTitle("DM Spam Complete")
                                    .setDescription(
                                        "1. Verifying user access ✅\n"
                                        "2. Opening DM channel ✅\n"
                                        "3. Sending messages ✅\n"
                                        "4. Finalizing results ✅"
                                    )
                                    .setFooter(fmt::format("Successfully spammed {}'s DMs!", user.username));

                                thread_data->edit_original_response(dpp::message(
                                    thread_data->command.channel_id, final_embed.build()
                                ));
                                return;
                            }

                            thread_cringe->cluster.message_create(
                                dpp::message(dm_channel.id, message),
                                [thread_data, messages_sent, count, message, dm_channel, user, send_next_message](
                                    const dpp::confirmation_callback_t& msg_cb) {
                                    if (!msg_cb.is_error()) {
                                        (*messages_sent)++;

                                        auto progress_embed = CringeEmbed::createLoading("DM Spam Operation", "")
                                            .setDescription(
                                                fmt::format(
                                                    "1. Verifying user access ✅\n"
                                                    "2. Opening DM channel ✅\n"
                                                    "3. Sending messages ({}/{}) ⏳\n"
                                                    "4. Finalizing results ⌛",
                                                    *messages_sent, count
                                                )
                                            )
                                            .setFooter(fmt::format("Spamming {}'s DMs...", user.username));

                                        thread_data->edit_original_response(dpp::message(
                                            thread_data->command.channel_id, progress_embed.build()
                                        ));

                                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                        (*send_next_message)();
                                    }
                                }
                            );
                        };

                        (*send_next_message)();
                    });
                });

            } catch (const std::exception &e) {
                thread_data->edit_original_response(dpp::message(
                    thread_data->command.channel_id,
                    CringeEmbed::createError(fmt::format("Error: {}", e.what())).build()
                ));
            }
        }).detach();

    } catch (const std::exception &e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError(fmt::format("Error: {}", e.what())).build()
        ));
    }
}