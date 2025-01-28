#include "commands/voice/say_command.h"
#include <cstdio>
#include <filesystem>
#include <fmt/format.h>
#include <random>
#include "utils/embed/cringe_embed.h"

#include "utils/audio/cringe_ffmpeg.h"

dpp::slashcommand say_declaration() {
	dpp::slashcommand command;
	command.set_name("say").set_description("Make the bot speak text using TTS");

	// Add text parameter
	command.add_option(dpp::command_option(dpp::co_string, "text", "Text to speak", true));

	// Add language parameter
	auto lang_option = dpp::command_option(dpp::co_string, "language", "TTS language", false);
	lang_option.add_choice(dpp::command_option_choice("English", "en"))
			.add_choice(dpp::command_option_choice("Spanish", "es"))
			.add_choice(dpp::command_option_choice("French", "fr"))
			.add_choice(dpp::command_option_choice("German", "de"))
			.add_choice(dpp::command_option_choice("Japanese", "ja"))
			.add_choice(dpp::command_option_choice("Korean", "ko"))
			.add_choice(dpp::command_option_choice("Russian", "ru"))
			.add_choice(dpp::command_option_choice("zallocchi", "zallocchi"))
			.add_choice(dpp::command_option_choice("Chinese", "zh"));

	command.add_option(lang_option);

	// Add filter option using your existing filters
	auto filter_option = dpp::command_option(dpp::co_string, "filter", "Audio filter to apply", false);
	for (const auto& [filter_name, _]: CringeFFMPEG::get_filters()) {
		filter_option.add_choice(dpp::command_option_choice(filter_name, filter_name));
	}
	command.add_option(filter_option);

	return command;
}

void say_command(CringeBot& cringe, const dpp::slashcommand_t& event) {
	event.thinking(true);

	try {
		// Get guild and check if user is in voice channel
		dpp::guild* guild = dpp::find_guild(event.command.guild_id);
		if (!guild) {
			throw std::runtime_error("Could not find guild");
		}

		// Check if user is in voice channel
		if (auto voice_state = guild->voice_members.find(event.command.member.user_id);
			voice_state == guild->voice_members.end()) {
			throw std::runtime_error("You must be in a voice channel");
		}

		// Get command parameters
		std::string temp_file;
		std::string text = std::get<std::string>(event.get_parameter("text"));
		std::string language = event.get_parameter("language").index() != 0
									   ? std::get<std::string>(event.get_parameter("language"))
									   : "en";
		if (true) {
			CURL* curl = curl_easy_init();
			if (!curl) {
				throw std::runtime_error("Failed to initialize CURL");
			}

			// Generate unique filename
			std::random_device rd;
			std::mt19937 gen(rd());
			temp_file = fmt::format("tts_{}.wav", gen());

			// Open file to write the streamed audio
			FILE* fp = fopen(temp_file.c_str(), "wb");
			if (!fp) {
				curl_easy_cleanup(curl);
				throw std::runtime_error("Failed to create output file");
			}

			// URL encode the text
			char* encoded_text = curl_easy_escape(curl, text.c_str(), text.length());
			if (!encoded_text) {
				fclose(fp);
				curl_easy_cleanup(curl);
				throw std::runtime_error("Failed to encode text");
			}

			// Construct the URL with query parameters
			std::string url = fmt::format(
					"http://192.168.0.25:7851/api/tts-generate-streaming"
					"?text={}&voice=zallocchi.wav&language=es&output_file={}",
					encoded_text, temp_file);

			curl_free(encoded_text);

			// Set up CURL options
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

			// Perform request
			CURLcode res = curl_easy_perform(curl);

			// Cleanup
			fclose(fp);
			curl_easy_cleanup(curl);

			if (res != CURLE_OK) {
				std::filesystem::remove(temp_file);
				throw std::runtime_error(fmt::format("AllTalk API request failed: {}", curl_easy_strerror(res)));
			}

			// Create song object for audio system
			CringeSong tts_song;
			tts_song.url = std::filesystem::absolute(temp_file).string();
			tts_song.title = "TTS Message";
			tts_song.is_tts = true;
		} else {
			// Generate unique filename for this TTS request
			std::random_device rd;
			std::mt19937 gen(rd());
			temp_file = fmt::format("tts_{}.mp3", gen());

			// Create gTTS command
			std::string gtts_cmd = fmt::format("gtts-cli --lang {} \"{}\" --output {}", language, text, temp_file);

			// Generate TTS file
			if (system(gtts_cmd.c_str()) != 0) {
				throw std::runtime_error("Failed to generate TTS audio");
			}
		}

		// Create song object for audio system
		CringeSong tts_song;
		tts_song.url = std::filesystem::absolute(temp_file).string();
		tts_song.title = "TTS Message";
		tts_song.is_tts = true;

		// Add filter if specified
		if (event.get_parameter("filter").index() != 0) {
			std::string filter = std::get<std::string>(event.get_parameter("filter"));
			tts_song.filter = std::vector<std::string>{filter};
		}

		// Get voice connection
		dpp::voiceconn* voice = event.from->get_voice(event.command.guild_id);

		// Join voice channel if not already connected
		if (!voice || !voice->voiceclient || !voice->voiceclient->is_ready()) {
			if (!guild->connect_member_voice(event.command.member.user_id)) {
				throw std::runtime_error("Failed to join voice channel");
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			voice = event.from->get_voice(event.command.guild_id);
		}

		// Create audio streamer if needed
		if (!cringe.audio_streamer) {
			cringe.audio_streamer = std::make_shared<CringeAudioStreamer>();
		}

		// Stream the TTS audio
		cringe.current_stream = cringe.audio_streamer->stream(voice, tts_song);

		// Send success message
		event.edit_original_response(
				dpp::message().add_embed(CringeEmbed::createSuccess("Speaking your message")
												 .setDescription(fmt::format("Speaking: *{}*", text))
												 .build()));

	} catch (const std::exception& e) {
		event.edit_original_response(
				dpp::message().add_embed(CringeEmbed::createError(fmt::format("Error: {}", e.what()))
												 .setTitle("Failed to Speak Message")
												 .build()));
	}
}
