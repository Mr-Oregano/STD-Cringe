#include "commands/voice/play_command.h"
#include <algorithm>
#include <chrono>
#include <fmt/format.h>
#include <future>
#include "dpp/dpp.h"
#include "utils/audio/cringe_audio.h"
#include "utils/audio/cringe_ffmpeg.h"
#include "utils/audio/cringe_youtube.h"
#include "utils/embed/cringe_embed.h"
#include "utils/misc/cringe_emoji.h"

/**
 * @brief Creates the slashcommand declaration for the play command
 * @return dpp::slashcommand
 *
 * Instantiates the slashcommand for the play command. This declaration
 * is used at bot startup to define the structure and context of the
 * play command. The command allows a user to:
 * - provide the track they wish to listen to (required)
 * - provide a filter (optional)
 * - provide a second filter (optional)
 */
dpp::slashcommand play_declaration() {
    dpp::slashcommand slashcommand = dpp::slashcommand().set_name("play").set_description("Stream a song in a voice channel");

	const auto song = dpp::command_option(dpp::co_string, "song", "Song you wish to stream (either query or URL)", true);
	auto filter1 = dpp::command_option(dpp::co_string, "filter1", "First audio filter");
	auto filter2 = dpp::command_option(dpp::co_string, "filter2", "Second audio filter");
    
    // Add choices for both filter options
    for (const auto& [filter_name, _] : CringeFFMPEG::get_filters()) {
        filter1.add_choice(dpp::command_option_choice(filter_name, filter_name));
        filter2.add_choice(dpp::command_option_choice(filter_name, filter_name));
    }
    
    return slashcommand.add_option(song).add_option(filter1).add_option(filter2);
}

/**
 * @brief The implementation of the `/play` command.
 * @param cringe A reference to the base CringeBot class
 * @param event A reference to the event in which invoked the command.
 * @return bool
 *
 * The function that is run when the user invokes the `/play` slashcommand.
 * It is used to stream audio from an external audio source to the active
 * voice channel of the bot. Filters can be passed in and merged together, and
 * these filters are applied via FFMPEG.
 * @throws std::exception If there's an error processing the command
 */
void play_command(CringeBot& cringe, const dpp::slashcommand_t& event) {
    try {
    	// Since the play command takes some time, we must tell Discord to wait and
    	// not throw away our request (otherwise it will be assumed a dead/stale req.)
        event.thinking();

    	// The initial embed, showing that the bot is creating initial integrations
        auto finding_embed = CringeEmbed::createLoading("Finding Audio Channel", "")
            .setDescription(
                fmt::format(
                    "{}   Processing audio status\n"
                    "{}   Preparing playback\n"
                    "{}   Starting stream",
                    CringeEmoji::spinner,
                    CringeEmoji::spinner,
                    CringeEmoji::spinner
                )
            )
            .setFooter("Step 1 of 3", event.from->creator->me.get_avatar_url());
		// Send the initial embed showing the loading state
        event.edit_original_response(
            dpp::message(event.command.channel_id, finding_embed.build())
        );

    	// Fetch the guild reference via the guild_id, throw if not found
        dpp::guild* guild = dpp::find_guild(event.command.guild_id);
        if (!guild) {
            throw std::runtime_error("Could not find guild");
        }

        // Get voice state, throw if user isn't currently in a voice channel
        if (auto voice_state = guild->voice_members.find(event.command.member.user_id);
            voice_state == guild->voice_members.end()) {
            throw std::runtime_error("You must be in a voice channel");
        }

    	// The song URL (or title) that was passed in by the user
        auto url = std::get<std::string>(event.get_parameter("song"));
		// The voice connection to a voice channel (bot can only be in 1 channel per guild)
    	dpp::voiceconn* voice = event.from->get_voice(event.command.guild_id);

        // Join voice channel if not already connected
        // TODO: Doesn't work right now. Fix callback
        if (!voice || !voice->voiceclient || !voice->voiceclient->is_ready()) {
            if (!guild->connect_member_voice(event.command.member.user_id)) {
                throw std::runtime_error("Failed to join voice channel");
            }

            // Wait briefly for voice connection
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            voice = event.from->get_voice(event.command.guild_id);
            if (!voice || !voice->voiceclient || !voice->voiceclient->is_ready()) {
                throw std::runtime_error("Failed to establish voice connection");
            }
        }

    	// Show loading state for fetching the song content from YTDLP
        auto fetching_embed = CringeEmbed::createLoading("Fetching Song Information", "")
            .setDescription(
                fmt::format(
                    "{}   Processing audio status\n"
                    "{}   Preparing playback\n"
                    "{}   Starting stream",
                    CringeEmoji::success,
                    CringeEmoji::spinner,
                    CringeEmoji::spinner
                )
            )
            .setFooter("Step 2 of 3", event.from->creator->me.get_avatar_url());

        event.edit_original_response(
            dpp::message(event.command.channel_id, fetching_embed.build())
        );

        // Get song information from YTDLP
        CringeYoutube youtube;
        CringeSong song = youtube.get_content(url);

    	// Show the song content as provided from YTDLP showing that song will begin straming
        auto streaming_embed = CringeEmbed::createLoading("Now Streaming", "")
            .setDescription(
                fmt::format(
                    "{}   Processing audio status\n"
                    "{}   Preparing playback\n"
                    "{}   Starting stream",
                    CringeEmoji::success,
                    CringeEmoji::success,
                    CringeEmoji::spinner
                )
            )
            .setFooter("Step 3 of 3", event.from->creator->me.get_avatar_url());

        event.edit_original_response(
            dpp::message(event.command.channel_id, streaming_embed.build())
        );

    	// Apply the filtes as selected by the user in the slashcommand invocation
        std::vector<std::string> selected_filters;
        auto options = event.command.get_command_interaction().options;
        
        // Check for filter1
        if (auto it = std::find_if(options.begin(), options.end(),
            [](const auto& opt) { return opt.name == "filter1"; }); 
            it != options.end()) {
            selected_filters.push_back(std::get<std::string>(it->value));
        }

        // Check for filter2
        if (auto it = std::find_if(options.begin(), options.end(),
            [](const auto& opt) { return opt.name == "filter2"; }); 
            it != options.end()) {
            selected_filters.push_back(std::get<std::string>(it->value));
        }

        // Set the combined filters
        if (!selected_filters.empty()) {
            song.filter = selected_filters;
        }

    	// Check if we are not currently playing audio and if the queue is empty. If
    	// this is true, process the audio immediately and begin playback instantly.
        if (!voice->voiceclient->is_playing() && cringe.queue_store->getQueueSize(event.command.guild_id) == 0) {
            // Create streamer if it doesn't exist in current Bot context
            if (!cringe.audio_streamer) {
                cringe.audio_streamer = std::make_shared<CringeAudioStreamer>();
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

            // Set up queue callback
            cringe.audio_streamer->set_queue_callback(
                [queue_store = cringe.queue_store](dpp::snowflake guild_id) {
                    return queue_store->getNextSong(guild_id);
                }
            );

            // Start playing
            cringe.current_stream = cringe.audio_streamer->stream(voice, song, play_next);

        	// Set the voice channel status to show what song is playing
        	cringe.cluster.channel_set_voice_status_sync(voice->channel_id, fmt::format("Listening to: {}", song.title));

        	// Create the final embed showing which song is streaming
            auto now_playing_embed = CringeEmbed()
                .setTitle("Now Streaming")
                .setThumbnail(CringeIcon::MusicIcon)
                .addField("Title", fmt::format("**[{}]({})**", song.title, song.url))
                .addField("Artist", fmt::format("*[{}]({})*", song.artist.value_or("Unknown"), song.url))
                .addField("Streaming in", dpp::find_channel(voice->channel_id)->get_mention(), true)
                .addField("Duration", song.get_duration_string(), true)
                .addField("Upload Date", 
                    song.upload_date ? 
                    fmt::format("<t:{}:D>", std::chrono::system_clock::to_time_t(song.upload_date.value())) : 
                    "Unknown", 
                    true)
                .setImage(song.thumbnail.value_or(""))
                .addField("Comments", 
                    song.comment_count ? std::to_string(song.comment_count.value()) : "Unknown", 
                    true)
                .addField("Views", song.get_view_count_string(), true)
                .addField("Subscribers", 
                    song.subscriber_count ? std::to_string(song.subscriber_count.value()) : "Unknown", 
                    true)
                .setColor(CringeColor::CringePrimary)
                .setFooter(
                    fmt::format("Requested by: {}", event.command.usr.global_name),
                    event.command.usr.get_avatar_url()
                )
                .setTimestamp(time(nullptr));

            event.edit_original_response(
                dpp::message(event.command.channel_id, now_playing_embed.build())
            );
        } else {
        	// At this point, we can assume that the voice connection is either
        	// _currently_ streaming, or that there are other songs in the queue.
        	// Therefore, we should add the requested song to the queue
            cringe.queue_store->addSong(event.command.guild_id, song);

        	// Create embed showing that the song has been added to the queue, and
        	// show position of the song within the queue
            auto queue_embed = CringeEmbed::createSuccess(
                fmt::format("Added **{}** to the queue (Position: {})", 
                    song.title,
                    cringe.queue_store->getQueueSize(event.command.guild_id))
            );
            event.edit_original_response(
                dpp::message(event.command.channel_id, queue_embed.build())
            );
        }
    } catch (const std::exception& e) {
    	// Catch any failures during play command that were thrown and display them
    	// as a new embed send back to the user who called the command.
        auto error_embed = CringeEmbed::createError(e.what());
        event.edit_original_response(
            dpp::message(event.command.channel_id, error_embed.build())
        );
    }
}