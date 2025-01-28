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

#include "commands/voice/skip_command.h"

#include <fmt/format.h>

#include "commands/voice/play_command.h"
#include "utils/embed/cringe_embed.h"
#include "utils/audio/cringe_audio.h"

dpp::slashcommand skip_declaration() {
    return dpp::slashcommand().set_name("skip").set_description(
        "Skip the song that is currently playing");
}

void skip_command(CringeBot& cringe, const dpp::slashcommand_t& event) {
    event.thinking(true);
    
    try {
        // Get voice connection
        dpp::voiceconn* voice = event.from->get_voice(event.command.guild_id);
        if (!voice || !voice->voiceclient || !voice->voiceclient->is_ready()) {
            throw std::runtime_error("Not connected to a voice channel");
        }
        std::cout << "Voice connection found\n";

        // Skip current song if playing
        if (cringe.audio_streamer) {
            cringe.audio_streamer->skip();
        }

        // Get next song from queue
        auto song = cringe.queue_store->getNextSong(event.command.guild_id);
        if (!song) {
            auto empty_embed = CringeEmbed::createSuccess("Queue is empty. Playback stopped.");
            event.edit_original_response(dpp::message(event.command.channel_id, empty_embed.build()));
            return;
        }

    	// Create callback that accepts StreamStatus and can call itself
    	std::function<void(StreamStatus)> play_next = [audio_streamer = cringe.audio_streamer,
													 queue_store = cringe.queue_store,
													 voice = voice,
													 guild_id = event.command.guild_id,
													 &play_next](StreamStatus status) {
    		// Only proceed if the song completed normally
    		if (status != StreamStatus::Completed) {
    			return;
    		}

    		if (auto next = queue_store->getNextSong(guild_id)) {
    			try {
    				audio_streamer->stream(voice, *next, play_next);
    			} catch (const std::exception& e) {
    				std::cout << "Error in play_next callback: " << e.what() << "\n";
    			}
    		}
        };

        // Start playing next song
        std::cout << "Starting to play: " << song->title << "\n";
        cringe.current_stream = cringe.audio_streamer->stream(voice, *song, play_next);
        
        auto success_embed = CringeEmbed::createSuccess("Skipped to next song.");
        event.edit_original_response(dpp::message(event.command.channel_id, success_embed.build()));
    } catch (const std::exception& e) {
        std::cerr << "Skip command error: " << e.what() << "\n";
        auto error_embed = CringeEmbed::createError(e.what());
        event.edit_original_response(dpp::message(event.command.channel_id, error_embed.build()));
    }
}