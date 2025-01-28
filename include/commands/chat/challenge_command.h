#ifndef CRINGE_CHALLENGE_COMMAND_H
#define CRINGE_CHALLENGE_COMMAND_H

#include <dpp/dpp.h>

class CringeBot;

struct ChallengePart {
    int part_number;
    std::string description;
    std::string answer;
    int points;
};

struct Challenge {
    dpp::snowflake challenge_id;
    std::string title;
    std::string description;
    std::vector<ChallengePart> parts;
};

dpp::slashcommand challenge_declaration();
void challenge_command(CringeBot &cringe, const dpp::slashcommand_t &event);
void handle_challenge_interaction(CringeBot& cringe, const dpp::interaction_create_t& event);

#endif