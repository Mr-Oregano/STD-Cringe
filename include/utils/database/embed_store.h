#ifndef CRINGE_EMBED_STORE_H
#define CRINGE_EMBED_STORE_H

#include "utils/database/cringe_database.h"
#include <dpp/dpp.h>
#include <optional>
#include <vector>
#include <string>

class EmbedStore {
private:
    CringeDB& db;
    void initTables();

public:
    explicit EmbedStore(CringeDB& database);

    // Store embed pages
    void storeEmbedPages(
        dpp::snowflake message_id,
        dpp::snowflake channel_id,
        const std::vector<dpp::embed>& pages,
        std::chrono::seconds lifetime = std::chrono::hours(24)
    );

    // Retrieve embed pages
    std::optional<std::vector<dpp::embed>> getEmbedPages(dpp::snowflake message_id);
    
    // Update current page
    void updateCurrentPage(dpp::snowflake message_id, size_t current_page);
    
    // Get current page
    size_t getCurrentPage(dpp::snowflake message_id);
    
    // Delete expired embeds
    void cleanupExpiredEmbeds();

private:
    // Helper methods for JSON conversion
    static nlohmann::json embedToJson(const dpp::embed& embed);
    static dpp::embed jsonToEmbed(const nlohmann::json& json);
};

#endif