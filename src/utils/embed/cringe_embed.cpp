#include "utils/embed/cringe_embed.h"
#include <fmt/format.h>

CringeEmbed::CringeEmbed() {
    initializeEmbed(CringeColor::CringePrimary);
}

CringeEmbed::CringeEmbed(const std::string& title, const std::string& description) {
    initializeEmbed(CringeColor::CringePrimary);
    setTitle(title);
    setDescription(description);
}

void CringeEmbed::initializeEmbed(uint32_t color, const std::string& icon) {
    embed = dpp::embed()
        .set_color(color)
        .set_timestamp(time(nullptr));
    
    if (!icon.empty()) {
        embed.set_thumbnail(icon);
    }
}

CringeEmbed& CringeEmbed::setTitle(const std::string& title) {
    embed.set_title(title);
    return *this;
}

CringeEmbed& CringeEmbed::setDescription(const std::string& description) {
    embed.set_description(description);
    return *this;
}

CringeEmbed& CringeEmbed::setColor(uint32_t color) {
    embed.set_color(color);
    return *this;
}

CringeEmbed& CringeEmbed::setThumbnail(const std::string& url) {
    embed.set_thumbnail(url);
    return *this;
}

CringeEmbed& CringeEmbed::setImage(const std::string& url) {
    embed.set_image(url);
    return *this;
}

CringeEmbed& CringeEmbed::setFooter(const std::string& text, const std::string& icon_url) {
    embed.set_footer(text, icon_url);
    return *this;
}

CringeEmbed& CringeEmbed::addField(const std::string& name, const std::string& value, bool inline_field) {
    if (value.length() >= 1024) {
        size_t chunk_max = 1024;
        for (size_t i = 0; i < value.length(); i += chunk_max) {
            std::string chunk = value.substr(i, chunk_max);
            std::string field_name = (i == 0) ? name : "";
            embed.add_field(field_name, chunk, inline_field);
        }
    } else {
        embed.add_field(name, value, inline_field);
    }
    return *this;
}

CringeEmbed& CringeEmbed::setTimestamp(time_t timestamp) {
    embed.set_timestamp(timestamp);
    return *this;
}

CringeEmbed& CringeEmbed::setAuthor(const std::string& name, const std::string& url, const std::string& icon_url) {
    embed.set_author(name, url, icon_url);
    return *this;
}

CringeEmbed& CringeEmbed::setUrl(const std::string& url) {
    embed.set_url(url);
    return *this;
}

CringeEmbed CringeEmbed::createSuccess(const std::string& description, const std::string& title) {
    CringeEmbed embed;
    embed.initializeEmbed(CringeColor::CringeSuccess, CringeIcon::SuccessIcon);
    return embed.setTitle(title).setDescription(description);
}

CringeEmbed CringeEmbed::createError(const std::string& description, const std::string& title) {
    CringeEmbed embed;
    embed.initializeEmbed(CringeColor::CringeError, CringeIcon::ErrorIcon);
    return embed.setTitle(title).setDescription(description);
}

CringeEmbed CringeEmbed::createWarning(const std::string& description, const std::string& title) {
    CringeEmbed embed;
    embed.initializeEmbed(CringeColor::CringeWarning, CringeIcon::WarningIcon);
    return embed.setTitle(title).setDescription(description);
}

CringeEmbed CringeEmbed::createInfo(const std::string& description, const std::string& title) {
    CringeEmbed embed;
    embed.initializeEmbed(CringeColor::CringePrimary, CringeIcon::InfoIcon);
    return embed.setTitle(title).setDescription(description);
}

CringeEmbed CringeEmbed::createLoading(const std::string& title, const std::string& description) {
    CringeEmbed embed;
    embed.initializeEmbed(CringeColor::CringePrimary, "https://app.btassociation.com/loading.gif");
    return embed.setTitle(title).setDescription(description);
}

CringeEmbed CringeEmbed::createCommand(const std::string& command_name, const std::string& description) {
    CringeEmbed embed;
    embed.initializeEmbed(CringeColor::CringeSecondary, CringeIcon::TerminalIcon);
    return embed.setTitle(fmt::format("/{}", command_name))
               .setDescription(description)
               .setFooter("Use /help for more information");
}

size_t CringeEmbed::calculateEmbedSize(const dpp::embed& embed) {
    size_t total = 0;
    
    if (!embed.title.empty()) total += embed.title.length();
    if (!embed.description.empty()) total += embed.description.length();
    if (!embed.footer->text.empty()) total += embed.footer->text.length();
    if (!embed.author->name.empty()) total += embed.author->name.length();
    
    for (const auto& field : embed.fields) {
        total += field.name.length() + field.value.length();
    }
    
    return total;
}

void CringeEmbed::splitDescription(std::vector<dpp::embed>& embeds, const dpp::embed& base_embed,
                                 const std::string& description, size_t max_chars) {
    size_t chunk_size = max_chars - 200; // Buffer for other embed elements
    
    for (size_t i = 0; i < description.length(); i += chunk_size) {
        dpp::embed page = base_embed;
        std::string chunk = description.substr(i, chunk_size);
        page.set_description(chunk);

        // Get the original footer text if it exists
        std::string original_footer = "";
        if (base_embed.footer && !base_embed.footer->text.empty()) {
            original_footer = base_embed.footer->text;
            // Remove any existing page numbers if present
            size_t page_pos = original_footer.find(" • Page ");
            if (page_pos != std::string::npos) {
                original_footer = original_footer.substr(0, page_pos);
            }
        }

        // Create new footer with both conversation ID and page numbers
        dpp::embed_footer footer;
        footer.text = fmt::format("{}{} • Page {}/{}",
            original_footer,
            original_footer.empty() ? "" : " ",
            embeds.size() + 1,
            (description.length() + chunk_size - 1) / chunk_size
        );
        page.set_footer(footer);
        embeds.push_back(page);
    }
}

std::vector<dpp::embed> CringeEmbed::buildPaginated(size_t max_chars) const {
    std::vector<dpp::embed> pages;
    
    if (calculateEmbedSize(embed) <= max_chars) {
        pages.push_back(embed);
        return pages;
    }
    
    // Create base embed without description and fields
    dpp::embed base = embed;
    base.description = "";
    base.fields.clear();
    
    // Split description if it exists
    if (!embed.description.empty()) {
        splitDescription(pages, base, embed.description, max_chars);
    }
    
    // Handle fields in the last page or create new pages if needed
    dpp::embed current_page = pages.empty() ? base : pages.back();
    bool has_changes = false;

    for (const auto& field : embed.fields) {
        size_t field_size = field.name.length() + field.value.length();

        if (calculateEmbedSize(current_page) + field_size > max_chars) {
            if (!pages.empty() && has_changes) {
                pages.back() = current_page;
            }
            current_page = base;
            current_page.add_field(field.name, field.value, field.is_inline);
            pages.push_back(current_page);  // Add the page immediately
            has_changes = false;
        } else {
            current_page.add_field(field.name, field.value, field.is_inline);
            has_changes = true;
        }
    }

    // Add the last page if it has any changes that weren't added
    if (has_changes) {
        pages.push_back(current_page);
    }

    // Update page numbers in footers
    for (size_t i = 0; i < pages.size(); i++) {
        dpp::embed_footer footer;
        footer.text = fmt::format("Page {}/{}", i + 1, pages.size());
        pages[i].set_footer(footer);
    }

    return pages;
}

// Helper function to send paginated embeds sequentially
void CringeEmbed::sendPaginatedEmbedsSequentially(const std::vector<dpp::embed>& pages,
                                   dpp::snowflake channel_id,
                                   dpp::cluster& cluster) {
    std::function<void(size_t)> send_next = [&](size_t index) {
        if (index >= pages.size()) return;

        dpp::message msg(channel_id, pages[index]);
        cluster.message_create(msg, [&send_next, index](const dpp::confirmation_callback_t& callback) {
            if (!callback.is_error()) {
                send_next(index + 1);
            }
        });
    };

    send_next(0);
}

dpp::embed CringeEmbed::build() const {
    return embed;
}

dpp::component CringeEmbed::createNavigationRow(size_t current_page, size_t total_pages) {
    // Create action row
    dpp::component action_row;
    action_row.set_type(dpp::cot_action_row);

    // Previous page button
    dpp::component prev_button;
    prev_button.set_type(dpp::cot_button)
               .set_label("previous")
               .set_style(dpp::cos_secondary)
               .set_id("prev_page")
               .set_disabled(current_page == 0);
    action_row.add_component(prev_button);

    // Page indicator
    dpp::component page_indicator;
    page_indicator.set_type(dpp::cot_button)
                  .set_style(dpp::cos_secondary)
                  .set_label(fmt::format("Page {}/{}", current_page + 1, total_pages))
                  .set_id("page_indicator")
                  .set_disabled(true);
    action_row.add_component(page_indicator);

    // Next page button
    dpp::component next_button;
    next_button.set_type(dpp::cot_button)
               .set_label("next")
               .set_style(dpp::cos_secondary)
               .set_id("next_page")
               .set_disabled(current_page >= total_pages - 1);
    action_row.add_component(next_button);

    return action_row;
}

void CringeEmbed::sendPaginatedEmbed(
    const std::vector<dpp::embed>& pages,
    dpp::snowflake channel_id,
    dpp::cluster& cluster,
    EmbedStore& embed_store,
    dpp::snowflake reply_to
) const {
    std::cout << "Sending paginated embed" << std::endl;
    if (pages.empty()) {
        std::cout << "No pages to send" << std::endl;
        return;
    }
    std::cout << "Pages to send: " << pages.size() << std::endl;

    // Create initial message with first page and navigation buttons
    dpp::message msg(channel_id, pages[0]);
    std::cout << "Creating initial message" << std::endl;
    msg.add_component(createNavigationRow(0, pages.size()));
    std::cout << "Navigation row added" << std::endl;

    // Set message reference if provided
    if (reply_to != 0) {
        std::cout << "Sending reply" << std::endl;
        cluster.channel_get(channel_id, [&cluster, &msg, reply_to, channel_id](const dpp::confirmation_callback_t& cb) {
            if (!cb.is_error()) {
                auto channel = std::get<dpp::channel>(cb.value);
                msg.set_reference(reply_to, channel.guild_id, channel_id);
                std::cout << "Message reference set with guild_id: " << channel.guild_id << std::endl;
            } else {
                msg.set_reference(reply_to);
                std::cout << "Channel fetch failed, using simple reference" << std::endl;
            }
        });
    }

    std::cout << "Sending initial message" << std::endl;
    // Send initial message
    cluster.message_create(msg, [&embed_store, pages, channel_id](const dpp::confirmation_callback_t& callback) {
        if (callback.is_error()) {
            std::cout << "Failed to send initial message" << std::endl;
            std::cout << callback.get_error().message << std::endl;
            return;
        }
        dpp::message sent_msg = std::get<dpp::message>(callback.value);
        std::cout << "Message sent" << std::endl;
        embed_store.storeEmbedPages(sent_msg.id, channel_id, pages);
        std::cout << "Embed pages stored" << std::endl;
    });
}