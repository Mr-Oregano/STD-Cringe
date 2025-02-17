cmake_minimum_required(VERSION 3.25)

set(BOT_NAME "cringe")

include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(WARNING "The file conan_toolchain.cmake doesn't exist, you have to run conan install first")
endif()

project(${BOT_NAME} VERSION 1.0 DESCRIPTION "std::cringe discord bot")

include(FetchContent)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(APPLE)
    print("This is the dumbest fucking OS on the planet")
endif()

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/utils/embed/cringe_embed.cpp
        src/utils/audio/cringe_queue.cpp
        src/commands/voice/join_command.cpp
        src/commands/voice/play_command.cpp
        src/commands/voice/queue_command.cpp
        src/commands/chat/info_command.cpp
        src/commands/chat/chat_command.cpp
        src/commands/chat/user_command.cpp
        src/commands/voice/skip_command.cpp
        include/commands/voice/skip_command.h
        src/commands/chat/confession_command.cpp
        include/commands/chat/confession_command.h
        include/commands/api/reddit_command.h
        src/commands/api/reddit_command.cpp
        src/commands/chat/imagine_command.cpp
        include/commands/chat/imagine_command.h
        include/commands/chat/chat_command.h
        include/commands/chat/user_command.h
        src/utils/listeners/cringe_slashcommand.cpp
        include/utils/listeners/cringe_slashcommand.h
        src/commands/chat/describe_command.cpp
        include/commands/chat/describe_command.h
        include/utils/misc/cringe_64.h
        src/utils/misc/cringe_64.cpp
        src/utils/validators/validate_chat.cpp
        src/utils/database/cringe_database.cpp
        include/commands/chat/info_command.h
        include/utils/audio/cringe_queue.h
        include/utils/audio/cringe_song.h
        include/utils/database/cringe_database.h
        include/utils/embed/cringe_embed.h
        src/utils/logger/logger.cpp
        include/utils/logger/logger.h
        include/utils/validators/validate_chat.h
        src/commands/chat/code_command.cpp
        include/commands/chat/code_command.h
        src/utils/audio/cringe_yt.cpp
        include/utils/audio/cringe_youtube.h
        src/utils/bot/cringe_bot.cpp
        include/utils/bot/cringe_bot.h
        src/utils/http/cringe_ollama.cpp
        include/utils/http/cringe_ollama.h
        src/utils/http/cringe_curl.cpp
        include/utils/http/cringe_curl.h
        src/utils/audio/cringe_ffmpeg.cpp
        include/utils/audio/cringe_ffmpeg.h
        src/utils/audio/cringe_audio.cpp
        include/utils/audio/cringe_audio.h
        include/utils/misc/cringe_color.h
        include/utils/misc/cringe_color.h
        include/utils/misc/cringe_icon.h
        src/utils/misc/cringe_pipe.cpp
        include/utils/misc/cringe_pipe.h
        include/utils/misc/cringe_thread_pool.h
        src/utils/audio/cringe_song.cpp 
         src/commands/voice/shuffle_command.cpp
         include/commands/voice/shuffle_command.h
        src/commands/voice/shuffle_command.cpp
        src/commands/chat/analytics_command.cpp
        include/commands/chat/analytics_command.h
        src/commands/chat/memory_command.cpp
        include/commands/chat/memory_command.h
        src/commands/chat/story_command.cpp
        include/commands/chat/story_command.h
        include/utils/database/message_store.h
        src/utils/database/message_store.cpp
        src/commands/test/test_embed.cpp
        include/commands/test/test_embed.h
        include/utils/database/embed_store.h
        src/utils/database/embed_store.cpp
        src/utils/database/story_store.cpp
        include/utils/database/story_store.h
        src/commands/chat/mimic_command.cpp
        include/commands/chat/mimic_command.h
        include/commands/test/sync_command.h
        src/commands/test/sync_command.cpp
        src/commands/chat/conspiracy_command.cpp
        include/commands/chat/conspiracy_command.h
        include/utils/database/conspiracy_store.h
        src/utils/database/conspiracy_store.cpp
        include/utils/http/cringe_openai.h
        src/utils/http/cringe_openai.cpp
        src/utils/misc/cringe_environment.cpp
        src/commands/chat/dm_command.cpp
        include/commands/chat/dm_command.h
        src/commands/chat/mock_command.cpp
        include/commands/chat/mock_command.h
        src/commands/test/test_command.cpp
        include/commands/test/test_command.h
        include/utils/misc/cringe_emoji.h
        src/utils/database/queue_store.cpp
        include/utils/database/queue_store.h
        src/commands/voice/queue_command.cpp
        include/commands/voice/queue_command.h
        src/commands/chat/challenge_command.cpp
        include/commands/chat/challenge_command.h
        include/utils/database/challenge_store.h
        src/utils/database/challenge_store.cpp
        include/utils/embed/challenge_embed.h
        src/utils/embed/challenge_embed.cpp
        include/utils/listeners/command_handler.h
        src/utils/listeners/command_handler.cpp
        include/commands/voice/music_player.h
        src/commands/voice/music_player.cpp
        include/utils/audio/player.h
        src/utils/audio/player.cpp
        include/utils/audio/track.h
        src/utils/audio/track.cpp
        # include/utils/audio/audio_processing.h
        # src/utils/audio/audio_processing.cpp
        src/commands/chat/impersonate_command.cpp
        include/commands/chat/impersonate_command.h
        src/commands/chat/jinx_command.cpp
        include/commands/chat/jinx_command.h
        src/utils/database/chat_store.cpp
        include/utils/database/chat_store.h
        src/utils/audio/ytdlp.cpp
        include/utils/audio/ytdlp.h
        src/utils/database/confession_store.cpp
        include/utils/database/confession_store.h
        src/utils/database/rage_store.cpp
        include/utils/database/rage_store.h
        src/commands/chat/ping_command.cpp
        include/commands/chat/ping_command.h
        include/commands/voice/say_command.h
        src/commands/voice/say_command.cpp
)

include(ExternalProject)
include(FindPackageHandleStandardArgs)
find_package(SQLite3 REQUIRED)
find_package(DPP REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus)
#find_package(OpenSSL REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(CURL REQUIRED)
find_library(FFMPEG_LIB avcodec avformat avutil avfilter)
if(CURL_FOUND)
    message("FOUND CURL!")
else()
    message("COULDNT FIND CURL")
endif()
if (DPP_FOUND)
    message("Found DPP! Version: ${DPP_VERSION}")
else ()
    message(WARNING "Could not find DPP install. Building from source instead.")
    option(DPP_BUILD_TEST "" OFF)
    FetchContent_Declare(
            libdpp
            GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
            GIT_TAG master)
    FetchContent_GetProperties(libdpp)
    if (NOT libdpp_POPULATED)
        FetchContent_Populate(libdpp)
        target_include_directories(${BOT_NAME} PUBLIC
                ${libdpp_SOURCE_DIR}/include
        )
        add_subdirectory(
                ${libdpp_SOURCE_DIR}
                ${libdpp_BINARY_DIR}
                EXCLUDE_FROM_ALL)
    endif ()
    FetchContent_Declare(fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG master
    )
    FetchContent_MakeAvailable(fmt)
    target_link_libraries(${BOT_NAME} dpp)
endif ()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_target_properties(${BOT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
target_include_directories(${BOT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
#        ${OPENSSL_INCLUDE_DIR}
        ${SQLite3_INCLUDE_DIRS}
        ${FFMPEG_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIR}
        ${DPP_INCLUDE_DIRS}
        ${OPUS_INCLUDE_DIRS}
)
string(ASCII 27 Esc)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
target_link_libraries(${BOT_NAME}
        dl
        ${CMAKE_THREAD_LIBS_INIT}
#        ${OPENSSL_CRYPTO_LIBRARY}
#        ${OPENSSL_SSL_LIBRARY}
        ${SQLite3_LIBRARIES}
        ${CURL_LIBRARIES}
        fmt::fmt
        spdlog::spdlog
        ${DPP_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${OPUS_LIBRARIES}
)
