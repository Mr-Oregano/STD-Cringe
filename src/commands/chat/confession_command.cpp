/*
 * MIT License
 *
 * Copyright (c) 2023 @nulzo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "commands/chat/confession_command.h"
#include "utils/embed/cringe_embed.h"

dpp::slashcommand confession_declaration() {
    return dpp::slashcommand()
        .set_name("confess")
        .set_description("Confess your sins (anonymously)")
        .add_option(dpp::command_option(dpp::co_string, "confession",
                                      "Your confession", true))
        .add_option(dpp::command_option(dpp::co_attachment, "image",
                                      "Add an image to your confession", false))
        .add_option(dpp::command_option(dpp::co_channel, "channel",
                                      "Channel to send the confession to (default: current channel)", false));
}

void confession_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
    event.thinking(true);
    
    try {
        std::string confession = std::get<std::string>(event.get_parameter("confession"));
        
        // Get target channel (default to current channel)
        dpp::snowflake channel_id = event.command.channel_id;
        if (event.get_parameter("channel").index() > 0) {
            channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
        }
        
        // Handle image attachment if provided
        std::string image_url;
        if (event.get_parameter("image").index() > 0) {
            dpp::snowflake image_id = std::get<dpp::snowflake>(event.get_parameter("image"));
            dpp::attachment attachment = event.command.get_resolved_attachment(image_id);
            
            // Validate the attachment is an image
            if (!ALLOWED_MIME_TYPES.contains(attachment.content_type)) {
                throw std::runtime_error("Only images (JPEG, PNG, GIF, WEBP) are allowed as attachments");
            }
            
            // Validate file size (e.g., 8MB limit)
            if (attachment.size > 8 * 1024 * 1024) {
                throw std::runtime_error("Image size must be less than 8MB");
            }
            
            image_url = attachment.url;
        }
        
        // Store confession in database
        dpp::snowflake confession_id = cringe.confession_store->createConfession(
            event.command.guild_id,
            channel_id,
            confession,
            image_url
        );
        
        // Create embed for the confession
        auto embed = CringeEmbed()
            .setTitle("Anonymous Confession #" + std::to_string(confession_id))
            .setDescription(confession)
            .setTimestamp(std::time(nullptr))
            .setFooter("Make an anonymous confession with /confess")
    		.setThumbnail(CringeIcon::ConfessionIcon);
            
        if (!image_url.empty()) {
            embed.setImage(image_url);
        }
        
        // Send confession to target channel
        cringe.cluster.message_create(dpp::message(channel_id, embed.build()));
        
        // Send confirmation to user
        auto reply_embed = CringeEmbed::createSuccess(
            fmt::format("Your confession has been sent to <#{}>!", channel_id)
        );
        event.edit_original_response(dpp::message(event.command.channel_id, reply_embed.build()));
        
    } catch (const std::exception &e) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            CringeEmbed::createError(fmt::format("Error: {}", e.what())).build()
        ));
    }
}