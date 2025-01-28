#ifndef CRINGE_LOGGER_H
#define CRINGE_LOGGER_H

#include <dpp/dpp.h>
#include <fmt/format.h>
#include <memory>
#include <mutex>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "utils/database/cringe_database.h"

enum class LogVerbosity {
    NONE,    // Only critical errors and essentials
    DEBUG,   // Normal debugging info
    VERBOSE  // Everything
};

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

class CringeLogger {
public:
    explicit CringeLogger(LogVerbosity verbosity = LogVerbosity::DEBUG);
    ~CringeLogger();

    // Prevent copying
    CringeLogger(const CringeLogger&) = delete;
    CringeLogger& operator=(const CringeLogger&) = delete;

    // For data referencing
    CringeDB* db{nullptr};

    // Core logging methods
    void log(LogLevel level, const std::string& message);
    void setLogLevel(LogLevel level);

    // DPP specific logging methods
    void logEvent(const dpp::log_t& event);
    void logSlashCommand(const dpp::slashcommand_t& event);
	void logMessageDelete(
		const std::string& channel_name,
		const std::string& author_name,
		const std::string& deleter_name,
		const std::string& content
	);
	void logMessage(const dpp::message_create_t& event);
    void logStart();

    // Verbosity methods
    void setVerbosity(LogVerbosity level);
    LogVerbosity getVerbosity() const;
	std::string formatLogTime();
    std::string last_logged_time;
    std::chrono::system_clock::time_point last_log_time;


	void setDatabase(CringeDB* database) {
		db = database;
	}
    

private:
    LogVerbosity verbosity;

    std::shared_ptr<spdlog::async_logger> logger;
    std::mutex logMutex;

    // Terminal colors
	const std::string MAGENTA = "\e[1;35m";
	const std::string COLOR_RESET = "\e[0m";
	const std::string GREEN = "\e[1;32m";
	const std::string YELLOW = "\e[1;33m";
	const std::string RED = "\e[1;31m";
	const std::string CYAN = "\e[1;36m";
	const std::string WHITE = "\e[1;37m";
    const std::string GRAY = "\e[38;5;240m";     // Dark gray
    const std::string BLUE = "\e[38;5;39m";      // Bright blue

    // Helper methods
    std::string colorMessage(const std::string& color, const std::string& message);
	std::string centerText(const std::string& text, size_t width) const;
	static LogLevel convertDppLogLevel(dpp::loglevel level);
    static spdlog::level::level_enum convertToSpdLogLevel(LogLevel level);

    // Add prettier formatting helpers
    std::string getTimestamp() const;
    std::string formatLogMessage(const std::string& level, const std::string& message);
    std::string createBox(const std::string& message, const std::string& color) const;
};

#endif