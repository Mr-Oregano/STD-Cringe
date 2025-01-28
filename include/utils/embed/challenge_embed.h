#ifndef CRINGE_CHALLENGE_EMBED_H
#define CRINGE_CHALLENGE_EMBED_H

#include "utils/embed/cringe_embed.h"
#include "utils/database/challenge_store.h"
#include <dpp/dpp.h>

class ChallengeEmbed {
public:
    static CringeEmbed createChallengeListEmbed(const std::vector<Challenge>& challenges);
    static CringeEmbed createChallengeDetailEmbed(const Challenge& challenge, dpp::snowflake user_id, CringeChallengeStore& store);
    static dpp::component createChallengeListRow(const std::vector<Challenge>& challenges);
    static dpp::component createChallengeDetailRow(const Challenge& challenge, int current_part);
    static std::string formatPoints(const Challenge& challenge);

private:
    static std::string formatProgress(const Challenge& challenge, dpp::snowflake user_id, CringeChallengeStore& store);
    static std::string formatPartStatus(const ChallengePart& part, bool unlocked, bool completed);
};

#endif