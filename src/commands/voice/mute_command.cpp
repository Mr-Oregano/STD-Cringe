#include "commands/voice/mute_command.h"

dpp::slashcommand mute_declaration() {
    return dpp::slashcommand()
        .set_name("trollmute")
        .set_description("Randomly mute people when they talk (admin only)")
        .add_option(dpp::command_option(dpp::co_boolean, "enabled", 
            "Enable or disable troll muting", true));
}

void mute_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(true);
    
    // Check if user has admin permissions
    if (!event.command.member.has_permission(dpp::p_administrator)) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            "Nice try, but only admins can use this command 😈"
        ));
        return;
    }
    
    bool enabled = std::get<bool>(event.get_parameter("enabled"));
    cringe.troll_mute_enabled[event.command.guild_id] = enabled;
    
    event.edit_original_response(dpp::message(
        event.command.channel_id,
        enabled ? "Troll muting enabled! 🤫" : "Troll muting disabled 😢"
    ));
}