#include "utils/logger/logger.h"

const size_t CHANNEL_WIDTH = 10; 
const size_t LEVEL_WIDTH = 8;

CringeLogger::CringeLogger(LogVerbosity verbosity) : verbosity(verbosity) {
    const int threadPoolSize = 8192;
    const int maxFileSize = 5242880;
    const int maxFiles = 10;
	last_log_time = std::chrono::system_clock::now();
	last_logged_time = "";
    const std::string logName = "std_cringe.log";
    
    std::vector<spdlog::sink_ptr> sinks;
    spdlog::init_thread_pool(threadPoolSize, 2);
    
    auto stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto rotatingSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logName, maxFileSize, maxFiles
    );
    
    sinks.push_back(stdoutSink);
    sinks.push_back(rotatingSink);
    
    logger = std::make_shared<spdlog::async_logger>(
        "cringe", 
        sinks.begin(), 
        sinks.end(), 
        spdlog::thread_pool(), 
        spdlog::async_overflow_policy::block
    );
    
    spdlog::register_logger(logger);
    
    // Beautiful pattern with colors and alignment
    logger->set_pattern("%^%v%$");
    // Set initial log level based on verbosity
    switch (verbosity) {
        case LogVerbosity::NONE:
            logger->set_level(spdlog::level::critical);
            break;
        case LogVerbosity::DEBUG:
            logger->set_level(spdlog::level::debug);
            break;
        case LogVerbosity::VERBOSE:
            logger->set_level(spdlog::level::trace);
            break;
    }
}

CringeLogger::~CringeLogger() = default;

std::string CringeLogger::colorMessage(const std::string& color, const std::string& message) {
    return fmt::format("{}{}{}", color, message, COLOR_RESET);
}

std::string CringeLogger::centerText(const std::string& text, size_t width) const {
    if (text.length() > width) {
        // Truncate and add ellipsis
        return text.substr(0, width - 3) + "...";
    }
    
    size_t padding = width - text.length();
    size_t left_pad = padding / 2;
    size_t right_pad = padding - left_pad;
    
    return std::string(left_pad, ' ') + text + std::string(right_pad, ' ');
}

void CringeLogger::setVerbosity(LogVerbosity level) {
    verbosity = level;
    switch (level) {
        case LogVerbosity::NONE:
            logger->set_level(spdlog::level::critical);
            break;
        case LogVerbosity::DEBUG:
            logger->set_level(spdlog::level::debug);
            break;
        case LogVerbosity::VERBOSE:
            logger->set_level(spdlog::level::trace);
            break;
    }
}

void CringeLogger::logEvent(const dpp::log_t& event) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::string level_str;
    std::string color;
    
    // Convert DPP log level to our format
    switch (event.severity) {
        case dpp::ll_trace:
            level_str = "trace";
            color = GRAY;
            break;
        case dpp::ll_debug:
            level_str = "debug";
            color = BLUE;
            break;
        case dpp::ll_info:
            level_str = "info";
            color = WHITE;
            break;
        case dpp::ll_warning:
            level_str = "warn";
            color = YELLOW;
            break;
        case dpp::ll_error:
            level_str = "error";
            color = RED;
            break;
        case dpp::ll_critical:
            level_str = "crit";
            color = RED;
            break;
        default:
            level_str = "info";
            color = WHITE;
            break;
    }

    // Format the log message with our consistent style
    std::string formatted_message = fmt::format("{} {} {} {} {} {}",
        formatLogTime(),
        colorMessage(color, fmt::format("{:<8}", level_str)),
        colorMessage(GRAY, "│"),
        colorMessage(CYAN, centerText("system", 12)),
        colorMessage(GRAY, "│"),
        event.message
    );

    // Log using the appropriate level
    switch (event.severity) {
        case dpp::ll_trace:
            logger->trace(formatted_message);
            break;
        case dpp::ll_debug:
            logger->debug(formatted_message);
            break;
        case dpp::ll_info:
            logger->info(formatted_message);
            break;
        case dpp::ll_warning:
            logger->warn(formatted_message);
            break;
        case dpp::ll_error:
            logger->error(formatted_message);
            break;
        case dpp::ll_critical:
            logger->critical(formatted_message);
            break;
        default:
            logger->info(formatted_message);
            break;
    }
}

void CringeLogger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    logger->log(convertToSpdLogLevel(level), message);
}

void CringeLogger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    logger->set_level(convertToSpdLogLevel(level));
}

void CringeLogger::logStart() {
    const int boxWidth = 100;
    fmt::print(
    	"┌{0:─^{2}}┐\n│{1: ^{2}}│\n│{3: ^{2}}│\n└{0:─^{2}}┘\n",
        "",
        colorMessage(MAGENTA, "std::cringe"),
        boxWidth,
        "version 0.0.1");
}

void CringeLogger::logSlashCommand(const dpp::slashcommand_t &event) {
	std::string channelSection = centerText(event.command.channel.name, 12);
	logger->info("{} │ {} used {}",
		channelSection,
		colorMessage(this->YELLOW, event.command.usr.username),
		colorMessage(this->MAGENTA, event.command.get_command_name()));
}

void CringeLogger::logMessageDelete(
    const std::string& channel_name,
    const std::string& author_name,
    const std::string& deleter_name,
    const std::string& content
) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (!content.empty()) {
        logger->warn("{} {} {} {} {} {} {}",
            formatLogTime(),
            colorMessage(YELLOW, fmt::format("{:<8}", "warn")),
            colorMessage(GRAY, "│"),
            colorMessage(CYAN, centerText(channel_name, CHANNEL_WIDTH)),
            colorMessage(GRAY, "│"),
            colorMessage(RED, fmt::format("Message from {} was deleted by {}", 
                author_name, deleter_name)),
            colorMessage(WHITE, "Content: " + content)
        );
    } else {
        logger->warn("{} {} {} {} {} {}",
            formatLogTime(),
            colorMessage(YELLOW, fmt::format("{:<8}", "warn")),
            colorMessage(GRAY, "│"),
            colorMessage(CYAN, centerText(channel_name, CHANNEL_WIDTH)),
            colorMessage(GRAY, "│"),
            colorMessage(RED, fmt::format("Message was deleted by {}", deleter_name))
        );
    }
}

void CringeLogger::logMessage(const dpp::message_create_t& event) {
    if (verbosity == LogVerbosity::NONE) return;
    
    std::lock_guard<std::mutex> lock(logMutex);
    if (!event.msg.author.is_bot()) {
        std::string channel_name = event.from->creator->channel_get_sync(event.msg.channel_id).name;
        std::string centered_channel = centerText(channel_name, CHANNEL_WIDTH);
        
        logger->debug("{} {} {} {} {} {} {}", 
            formatLogTime(),
            colorMessage(BLUE, fmt::format("{:<8}", "debug")),
            colorMessage(GRAY, "│"),
            colorMessage(CYAN, fmt::format(" {} ", centered_channel)),
            colorMessage(GRAY, "│"),
            colorMessage(YELLOW, event.msg.author.username),
            colorMessage(WHITE, "said " + event.msg.content));
    }
}

LogLevel CringeLogger::convertDppLogLevel(dpp::loglevel level) {
    switch (level) {
        case dpp::ll_trace: return LogLevel::TRACE;
        case dpp::ll_debug: return LogLevel::DEBUG;
        case dpp::ll_info: return LogLevel::INFO;
        case dpp::ll_warning: return LogLevel::WARN;
        case dpp::ll_error: return LogLevel::ERROR;
        case dpp::ll_critical: return LogLevel::CRITICAL;
        default: return LogLevel::INFO;
    }
}

spdlog::level::level_enum CringeLogger::convertToSpdLogLevel(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return spdlog::level::trace;
        case LogLevel::DEBUG: return spdlog::level::debug;
        case LogLevel::INFO: return spdlog::level::info;
        case LogLevel::WARN: return spdlog::level::warn;
        case LogLevel::ERROR: return spdlog::level::err;
        case LogLevel::CRITICAL: return spdlog::level::critical;
        default: return spdlog::level::info;
    }
}

std::string CringeLogger::formatLogTime() {
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
	std::string current_time = ss.str();

	// If it's been more than 2 seconds since the last log, or the time is different
	if (current_time != last_logged_time) {
		last_logged_time = current_time;
		last_log_time = now;
		return fmt::format("{}[{}]{}", GRAY, current_time, COLOR_RESET);
	}

	// Return spaces to maintain alignment
	return fmt::format("{} {} {}", GRAY, std::string(current_time.length(), ' '), COLOR_RESET);
}