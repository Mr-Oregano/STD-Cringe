#ifndef CRINGE_USER_STORE_H
#define CRINGE_USER_STORE_H

#include <dpp/dpp.h>
#include "utils/database/cringe_database.h"
#include <chrono>
#include <optional>
#include <vector>


/**
 * @brief Represents user data stored in the database
 */
struct UserData {
    dpp::snowflake id;
    std::string username;
    std::string discriminator;
    std::string global_name;
    std::string avatar;
    std::string mention;
    std::string created_at;
    bool is_bot;
    bool is_system;
};

class UserStore {
private:
    CringeDB &db;
    void initTables();

public:
    explicit UserStore(CringeDB &database);
    void storeUser(const dpp::user &user);
    void storeUsers(const std::vector<dpp::user> &users);
    std::optional<UserData> getUser(dpp::snowflake user_id);
    std::vector<UserData> getGuildUsers(dpp::snowflake guild_id);
    void updateLastSeen(dpp::snowflake user_id);
};

#endif