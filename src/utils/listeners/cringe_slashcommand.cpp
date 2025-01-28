#include <functional>

#include "commands/api/reddit_command.h"
#include "commands/chat/chat_command.h"
#include "commands/chat/code_command.h"
#include "commands/chat/confession_command.h"
#include "commands/chat/describe_command.h"
#include "commands/chat/imagine_command.h"
#include "commands/chat/info_command.h"
#include "commands/chat/user_command.h"
#include "commands/voice/join_command.h"
#include "commands/voice/play_command.h"
#include "commands/voice/queue_command.h"
#include "commands/voice/skip_command.h"
#include "utils/misc/cringe_thread_pool.h"
#include "utils/bot/cringe_bot.h"
#include "commands/voice/shuffle_command.h"
// #include "commands/chat/analytics_command.h"
#include "commands/chat/analytics_command.h"
#include "commands/chat/challenge_command.h"
#include "commands/chat/conspiracy_command.h"
#include "commands/chat/dm_command.h"
#include "commands/chat/impersonate_command.h"
#include "commands/chat/jinx_command.h"
#include "commands/chat/memory_command.h"
#include "commands/chat/mimic_command.h"
#include "commands/chat/mock_command.h"
#include "commands/chat/story_command.h"
#include "commands/test/sync_command.h"
#include "commands/test/test_command.h"
#include "commands/test/test_embed.h"
#include "commands/chat/ping_command.h"
#include "commands/voice/say_command.h"

namespace {
    // Thread pool for handling commands
    ThreadPool command_pool(4); // Adjust thread count based on needs

    template<typename F>
    void execute_async_command(F&& func, const dpp::slashcommand_t& event) {
        try {
            auto future = command_pool.enqueue(std::forward<F>(func));
            // Set a timeout for long-running commands
            if (future.wait_for(std::chrono::seconds(30)) == std::future_status::timeout) {
                event.edit_original_response(
                    dpp::message(event.command.channel_id, 
                        "Command timed out. Please try again.")
                );
            }
        } catch (const std::exception& e) {
            event.edit_original_response(
                dpp::message(event.command.channel_id, 
                    fmt::format("Command failed: {}", e.what()))
            );
        }
    }
}


void process_slashcommand(const dpp::slashcommand_t& event, CringeBot& cringe) {
    const auto& cmd_name = event.command.get_command_name();
    
    try {
        if (cmd_name == "info") {
            info_command(cringe, event);
        }
        else if (cmd_name == "user") {
            user_command(cringe, event);
        }
        else if (cmd_name == "chat") {
            execute_async_command([&cringe, event]() {
                chat_command(cringe, event);
            }, event);
        }
        else if (cmd_name == "code") {
            execute_async_command([&cringe, event]() {
                code_command(cringe, event);
            }, event);
        }
        else if (cmd_name == "shuffle") {
            shuffle_command(cringe, event);
        }
        else if (cmd_name == "join") {
            join_command(cringe, event);
        }
        else if (cmd_name == "play") {
            execute_async_command([&cringe, event]() {
                play_command(cringe, event);
            }, event);
        }
        else if (cmd_name == "queue") {
            queue_command(cringe, event);
        }
        else if (cmd_name == "skip") {
            skip_command(cringe, event);
        }
        else if (cmd_name == "confess") {
            confession_command(cringe, event);
        }
        else if (cmd_name == "reddit") {
            execute_async_command([&cringe, event]() {
                reddit_command(cringe, event);
            }, event);
        }
        else if (cmd_name == "imagine") {
            execute_async_command([&cringe, event]() {
                image_command(cringe, event);
            }, event);
        }
        else if (cmd_name == "describe") {
            execute_async_command([&cringe, event]() {
                describe_command(cringe, event);
            }, event);
        }
        else if (cmd_name == "mimic") {
            mimic_command(cringe, event);
        }
        else if (cmd_name == "sync") {
            sync_command(cringe, event);
        }
        else if (cmd_name == "analytics") {
            analytics_command(cringe, event);
        }
        else if (cmd_name == "memory") {
            memory_command(cringe, event);
        }
        else if (cmd_name == "story") {
            story_command(cringe, event);
        }
        else if (cmd_name == "test_embed") {
            test_embed_command(cringe, event);
        }
        else if (cmd_name == "conspiracy") {
            conspiracy_command(cringe, event);
        }
        else if (cmd_name == "dm") {
            dm_command(cringe, event);
        }
        else if (cmd_name == "mock") {
            mock_command(cringe, event);
        }
        else if (cmd_name == "test") {
            test_command(cringe, event);
        }
        else if (cmd_name == "challenge") {
            challenge_command(cringe, event);
        }
        else if (cmd_name == "impersonate") {
            impersonate_command(cringe, event);
        }
        else if (cmd_name == "jinx") {
            jinx_command(cringe, event);
        }
        else if (cmd_name == "ping") {
            ping_command(cringe, event);
        }
    	else if (cmd_name == "say") {
    		say_command(cringe, event);
    	}
    } catch (const std::exception& e) {
		CringeLogger logger;
		logger.log(LogLevel::ERROR, fmt::format("Command failed: {}", e.what()));
        event.edit_original_response(
            dpp::message(event.command.channel_id, 
                fmt::format("Command failed: {}", e.what()))
        );
    }
}

void register_slashcommands(CringeBot &cringe) {
    std::vector<dpp::slashcommand> commands{{
        info_declaration(),
        user_declaration(),
        chat_declaration(cringe),
        join_declaration(),
        code_declaration(),
        play_declaration(),
        queue_declaration(),
        skip_declaration(),
        confession_declaration(),
        reddit_declaration(),
        image_declaration(),
        describe_declaration(),
        shuffle_declaration(),
        analytics_declaration(),
        memory_declaration(),
        story_declaration(),
        test_embed_declaration(),
        mimic_declaration(),
        sync_declaration(),
        conspiracy_declaration(),
        dm_declaration(),
        mock_declaration(),
        test_declaration(),
        challenge_declaration(),
        impersonate_declaration(),
        jinx_declaration(),
        ping_declaration(),
    	say_declaration(),
    }};
    cringe.cluster.global_bulk_command_create(commands);
}