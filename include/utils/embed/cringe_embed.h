#ifndef CRINGE_EMBED_H
#define CRINGE_EMBED_H

#include <dpp/dpp.h>
#include <string>
#include <vector>
#include <memory>
#include "utils/misc/cringe_color.h"
#include "utils/misc/cringe_icon.h"
#include <numeric>

#include "utils/database/embed_store.h"

class CringeEmbed {
public:
    // Constructors
    CringeEmbed();
    explicit CringeEmbed(const std::string& title, const std::string& description = "");
    virtual ~CringeEmbed() = default;

    // Core embed building methods
    CringeEmbed& setTitle(const std::string& title);
    CringeEmbed& setDescription(const std::string& description);
    CringeEmbed& setColor(uint32_t color);
    CringeEmbed& setThumbnail(const std::string& url);
    CringeEmbed& setImage(const std::string& url);
    CringeEmbed& setFooter(const std::string& text, const std::string& icon_url = "");
    CringeEmbed& addField(const std::string& name, const std::string& value, bool inline_field = false);
    CringeEmbed& setTimestamp(time_t timestamp = time(nullptr));
    CringeEmbed& setAuthor(const std::string& name, const std::string& url = "", const std::string& icon_url = "");
    CringeEmbed& setUrl(const std::string& url);

    // Factory methods for different embed types
    static CringeEmbed createSuccess(const std::string& description, const std::string& title = "Success");
    static CringeEmbed createError(const std::string& description, const std::string& title = "Error");
    static CringeEmbed createWarning(const std::string& description, const std::string& title = "Warning");
    static CringeEmbed createInfo(const std::string& description, const std::string& title = "Information");
    static CringeEmbed createLoading(const std::string &title, const std::string &description);
    static CringeEmbed createCommand(const std::string& command_name, const std::string& description);

    // Build method
    [[nodiscard]] dpp::embed build() const;

    // Helper method for pagination
    std::vector<dpp::embed> buildPaginated(size_t max_chars = 2000) const;
    static size_t calculateEmbedSize(const dpp::embed& embed);
    void sendPaginatedEmbed(
        const std::vector<dpp::embed>& pages,
		dpp::snowflake channel_id,
        dpp::cluster& cluster,
        EmbedStore& embed_store,
        dpp::snowflake reply_to = 0  // New parameter
    ) const;
    static void sendPaginatedEmbedsSequentially(const std::vector<dpp::embed>& pages, dpp::snowflake channel_id, dpp::cluster& cluster);
    static dpp::component createNavigationRow(size_t current_page, size_t total_pages);

private:
    dpp::embed embed;
    
    // Helper method for initialization
    void initializeEmbed(uint32_t color, const std::string& icon = "");
    static void splitDescription(std::vector<dpp::embed>& embeds, const dpp::embed& base_embed, const std::string& description, size_t max_chars);
};

#endif // CRINGE_EMBED_H