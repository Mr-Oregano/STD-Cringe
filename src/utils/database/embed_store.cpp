#include "utils/database/embed_store.h"
#include <fmt/format.h>
#include <chrono>

using json = nlohmann::json;

EmbedStore::EmbedStore(CringeDB& database) : db(database) {
    initTables();
}

void EmbedStore::initTables() {
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS embed_pages (
            message_id INTEGER PRIMARY KEY,
            channel_id INTEGER NOT NULL,
            current_page INTEGER DEFAULT 0,
            total_pages INTEGER NOT NULL,
            pages TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            expires_at INTEGER NOT NULL
        )
    )");

    // Create index for cleanup
    db.createIndex("idx_embed_expires", "embed_pages", "expires_at");
}

json EmbedStore::embedToJson(const dpp::embed& embed) {
    json embed_json;
    
    // Basic embed properties
    embed_json["title"] = embed.title;
    embed_json["description"] = embed.description;
    if (embed.color.has_value()) {
        embed_json["color"] = embed.color.value();
    }
    embed_json["url"] = embed.url;

    // Fields
    json fields_json = json::array();
    for (const auto& field : embed.fields) {
        fields_json.push_back({
            {"name", field.name},
            {"value", field.value},
            {"inline", field.is_inline}
        });
    }
    embed_json["fields"] = fields_json;

    // Footer
    if (embed.footer.has_value()) {
        embed_json["footer"] = {
            {"text", embed.footer->text},
            {"icon_url", embed.footer->icon_url}
        };
    }

    // Author
    if (embed.author.has_value()) {
        embed_json["author"] = {
            {"name", embed.author->name},
            {"url", embed.author->url},
            {"icon_url", embed.author->icon_url}
        };
    }

    // Thumbnail and Image
    if (embed.thumbnail.has_value()) {
        embed_json["thumbnail"] = {
            {"url", embed.thumbnail->url}
        };
    }

    if (embed.image.has_value()) {
        embed_json["image"] = {
            {"url", embed.image->url}
        };
    }
    
    return embed_json;
}

dpp::embed EmbedStore::jsonToEmbed(const json& json) {
    dpp::embed embed;
    
    // Basic properties
    if (json.contains("title")) embed.set_title(json["title"].get<std::string>());
    if (json.contains("description")) embed.set_description(json["description"].get<std::string>());
    if (json.contains("color")) embed.set_color(json["color"].get<uint32_t>());
    if (json.contains("url")) embed.set_url(json["url"].get<std::string>());

    // Fields
    if (json.contains("fields")) {
        for (const auto& field : json["fields"]) {
            embed.add_field(
                field["name"].get<std::string>(),
                field["value"].get<std::string>(),
                field.value("inline", false)
            );
        }
    }

    // Footer
    if (json.contains("footer")) {
        embed.set_footer(
            json["footer"]["text"].get<std::string>(),
            json["footer"].value("icon_url", ""));
    }

    // Author
    if (json.contains("author")) {
        embed.set_author(
            json["author"]["name"].get<std::string>(),
            json["author"].value("url", ""),
            json["author"].value("icon_url", "")
        );
    }

    // Thumbnail and Image
    if (json.contains("thumbnail")) {
        embed.set_thumbnail(json["thumbnail"]["url"].get<std::string>());
    }

    if (json.contains("image")) {
        embed.set_image(json["image"]["url"].get<std::string>());
    }
    
    return embed;
}

void EmbedStore::storeEmbedPages(
    dpp::snowflake message_id,
    dpp::snowflake channel_id,
    const std::vector<dpp::embed>& pages,
    std::chrono::seconds lifetime
) {
    json pages_json = json::array();
    for (const auto& embed : pages) {
        pages_json.push_back(embedToJson(embed));
    }

    auto now = std::chrono::system_clock::now();
    auto expires = now + lifetime;

    db.execute(
        "INSERT OR REPLACE INTO embed_pages "
        "(message_id, channel_id, total_pages, pages, created_at, expires_at) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {
            std::to_string(message_id),
            std::to_string(channel_id),
            std::to_string(pages.size()),
            pages_json.dump(),
            std::to_string(std::chrono::system_clock::to_time_t(now)),
            std::to_string(std::chrono::system_clock::to_time_t(expires))
        }
    );
}

std::optional<std::vector<dpp::embed>> EmbedStore::getEmbedPages(dpp::snowflake message_id) {
    auto results = db.query(
        "SELECT pages FROM embed_pages "
        "WHERE message_id = ? AND expires_at > ?",
        {
            std::to_string(message_id),
            std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
        }
    );

    if (results.empty() || results[0].empty()) {
        return std::nullopt;
    }

    std::vector<dpp::embed> pages;
    json pages_json = json::parse(results[0][0]);
    
    for (const auto& embed_json : pages_json) {
        pages.push_back(jsonToEmbed(embed_json));
    }

    return pages;
}

void EmbedStore::updateCurrentPage(dpp::snowflake message_id, size_t current_page) {
    db.execute(
        "UPDATE embed_pages SET current_page = ? WHERE message_id = ?",
        {
            std::to_string(current_page),
            std::to_string(message_id)
        }
    );
}

size_t EmbedStore::getCurrentPage(dpp::snowflake message_id) {
    auto results = db.query(
        "SELECT current_page FROM embed_pages WHERE message_id = ?",
        {std::to_string(message_id)}
    );

    if (results.empty() || results[0].empty()) {
        return 0;
    }

    return std::stoull(results[0][0]);
}

void EmbedStore::cleanupExpiredEmbeds() {
    db.execute(
        "DELETE FROM embed_pages WHERE expires_at < ?",
        {std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))}
    );
}