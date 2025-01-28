#include "utils/embed/challenge_embed.h"
#include <fmt/format.h>

CringeEmbed ChallengeEmbed::createChallengeListEmbed(const std::vector<Challenge>& challenges) {
    auto embed = CringeEmbed()
        .setTitle("🎯 Coding Challenges")
        .setDescription("Select a challenge to view details or use the navigation buttons to browse more challenges.");

    for (const auto& challenge : challenges) {
        std::string field_value = fmt::format(
            "{}\n**Points:** {}\n**Parts:** {}",
            challenge.description,
            formatPoints(challenge),
            challenge.parts.size()
        );
        
        embed.addField(
            fmt::format("#{} {}", challenge.challenge_id, challenge.title),
            field_value,
            false
        );
    }

    return embed;
}

CringeEmbed ChallengeEmbed::createChallengeDetailEmbed(
    const Challenge& challenge,
    dpp::snowflake user_id,
    CringeChallengeStore& store
) {
    auto embed = CringeEmbed()
        .setTitle(fmt::format("#{} {}", challenge.challenge_id, challenge.title))
        .setDescription(challenge.description);

    // Add progress overview
    embed.addField("Progress", formatProgress(challenge, user_id, store), false);

    // Add parts
    bool previous_completed = true; // First part is always unlocked
    for (const auto& part : challenge.parts) {
        bool completed = store.isPartCompleted(user_id, challenge.challenge_id, part.part_number);
        std::string status = formatPartStatus(part, previous_completed, completed);
        
        embed.addField(
            fmt::format("Part {} ({} points)", part.part_number, part.points),
            status,
            false
        );
        
        previous_completed = completed;
    }

    return embed;
}

dpp::component ChallengeEmbed::createChallengeListRow(const std::vector<Challenge>& challenges) {
    dpp::component action_row;
    action_row.set_type(dpp::cot_action_row);

    dpp::component select_menu;
    select_menu.set_type(dpp::cot_selectmenu)
              .set_placeholder("Select a challenge to view details")
              .set_id("challenge_select");
    
    for (const auto& challenge : challenges) {
        select_menu.add_select_option(
            dpp::select_option()
                .set_label(fmt::format("#{} {}", challenge.challenge_id, challenge.title))
                .set_value(std::to_string(challenge.challenge_id))
                .set_description(challenge.description.substr(0, 100))
        );
    }
    
    action_row.add_component(select_menu);
    return action_row;
}

dpp::component ChallengeEmbed::createChallengeDetailRow(const Challenge& challenge, int current_part) {
    dpp::component action_row;
    action_row.set_type(dpp::cot_action_row);

    // Back to list button
    dpp::component back_button;
    back_button.set_type(dpp::cot_button)
               .set_label("Back to List")
               .set_style(dpp::cos_secondary)
               .set_id("challenge_back");
    action_row.add_component(back_button);

    // Submit answer button
    dpp::component submit_button;
    submit_button.set_type(dpp::cot_button)
                .set_label("Submit Answer")
                .set_style(dpp::cos_success)
                .set_id(fmt::format("challenge_submit_{}", challenge.challenge_id));
    action_row.add_component(submit_button);

    return action_row;
}

std::string ChallengeEmbed::formatPoints(const Challenge& challenge) {
    int total_points = 0;
    for (const auto& part : challenge.parts) {
        total_points += part.points;
    }
    
    return fmt::format("🏆 {} points total ({} parts)", 
        total_points, 
        challenge.parts.size()
    );
}

std::string ChallengeEmbed::formatProgress(const Challenge& challenge, dpp::snowflake user_id, CringeChallengeStore& store) {
    int completed_parts = 0;
    int earned_points = 0;
    
    for (const auto& part : challenge.parts) {
        if (store.isPartCompleted(user_id, challenge.challenge_id, part.part_number)) {
            completed_parts++;
            earned_points += part.points;
        }
    }
    
    return fmt::format(
        "Progress: {}/{} parts completed\n"
        "Points earned: {}/{} points\n"
        "Status: {}",
        completed_parts,
        challenge.parts.size(),
        earned_points,
        formatPoints(challenge),
        completed_parts == challenge.parts.size() ? "✅ Completed!" : 
        completed_parts == 0 ? "🆕 Not started" : "⏳ In progress"
    );
}

std::string ChallengeEmbed::formatPartStatus(const ChallengePart& part, bool unlocked, bool completed) {
    std::string status_emoji;
    if (completed) {
        status_emoji = "✅";
    } else if (unlocked) {
        status_emoji = "⭐";
    } else {
        status_emoji = "🔒";
    }
    
    if (!unlocked) {
        return fmt::format("{} Complete previous parts to unlock", status_emoji);
    }
    
    return fmt::format("{} {}\n\n**Points:** {}", 
        status_emoji,
        part.description,
        part.points
    );
}