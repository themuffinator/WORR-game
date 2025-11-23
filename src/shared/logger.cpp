#include "logger.hpp"

#include <atomic>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <format>
#include <mutex>
#include <string>
#include <string_view>

namespace worr {
namespace {

std::string g_module_name;
std::atomic<LogLevel> g_log_level = LogLevel::Info;
std::mutex g_logger_mutex;
void (*g_print_sink)(const char*) = nullptr;
void (*g_error_sink)(const char*) = nullptr;

struct LoggerConfig {
	std::string module_name;
	void (*print_sink)(const char*);
	void (*error_sink)(const char*);
};

/*
=============
ParseLogLevel

Parse the provided environment value into a LogLevel.
=============
*/
LogLevel ParseLogLevel(std::string_view value)
{
	std::string lowered(value);
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (lowered == "trace")
		return LogLevel::Trace;
	if (lowered == "debug")
		return LogLevel::Debug;
	if (lowered == "warn" || lowered == "warning")
		return LogLevel::Warn;
	if (lowered == "error")
		return LogLevel::Error;

	return LogLevel::Info;
}

/*
=============
ReadLogLevelFromEnv

Retrieve the log level from WORR_LOG_LEVEL or return the default.
=============
*/
LogLevel ReadLogLevelFromEnv()
{
	const char* env_value = std::getenv("WORR_LOG_LEVEL");
	if (!env_value)
		return LogLevel::Info;

	return ParseLogLevel(env_value);
}

/*
=============
LevelWeight

Assign a numeric weight to a log level for comparison.
=============
*/
int LevelWeight(LogLevel level)
{
	switch (level) {
	case LogLevel::Trace:
		return 0;
	case LogLevel::Debug:
		return 1;
	case LogLevel::Info:
		return 2;
	case LogLevel::Warn:
		return 3;
	case LogLevel::Error:
	default:
		return 4;
	}
}

/*
=============
EnsureSink

Call the provided sink if it exists.
=============
*/
void EnsureSink(void (*sink)(const char*), const std::string& message)
{
	if (sink)
		sink(message.c_str());
}

/*
=============
FormatMessage

Build a structured log message for output.
=============
*/
std::string FormatMessage(LogLevel level, std::string_view module_name, std::string_view message)
{
	static constexpr std::array prefixes{ "[TRACE]", "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]" };
	const size_t prefix_index = static_cast<size_t>(LevelWeight(level));
	std::string_view level_label = prefixes[std::min(prefix_index, prefixes.size() - 1)];

	std::string formatted = std::format("[WORR][{}] {} {}", module_name, level_label, message);
	if (!formatted.empty() && formatted.back() != '\n')
		formatted.push_back('\n');

	return formatted;
}

/*
=============
GetLoggerConfig

Capture current logger configuration under mutex protection.
=============
*/
LoggerConfig GetLoggerConfig()
{
	std::scoped_lock lock(g_logger_mutex);

	return { g_module_name, g_print_sink, g_error_sink };
}

} // namespace

/*
=============
InitLogger

Initialize the logger with module metadata and output sinks.
=============
*/
void InitLogger(std::string_view module_name, void (*print_sink)(const char*), void (*error_sink)(const char*))
{
	std::scoped_lock lock(g_logger_mutex);

	g_module_name = module_name;
	g_print_sink = print_sink;
	g_error_sink = error_sink;
	g_log_level.store(ReadLogLevelFromEnv(), std::memory_order_relaxed);
}

/*
=============
SetLogLevel

Override the current logging level programmatically.
=============
*/
void SetLogLevel(LogLevel level)
{
	g_log_level.store(level, std::memory_order_relaxed);
}

/*
=============
GetLogLevel

Fetch the currently active log level.
=============
*/
LogLevel GetLogLevel()
{
	return g_log_level.load(std::memory_order_relaxed);
}

/*
=============
IsLogLevelEnabled

Return whether the provided log level should emit output.
=============
*/
bool IsLogLevelEnabled(LogLevel level)
{
	const LogLevel current_level = g_log_level.load(std::memory_order_relaxed);
	return LevelWeight(level) >= LevelWeight(current_level);
}

/*
=============
LoggerPrint

Hook-compatible printer that respects the configured log level.
=============
*/
void LoggerPrint(const char* message)
{
	if (IsLogLevelEnabled(LogLevel::Info))
		Log(LogLevel::Info, message);
}

/*
=============
LoggerError

Hook-compatible error printer that always emits output.
=============
*/
void LoggerError(const char* message)
{
	const LoggerConfig config = GetLoggerConfig();

	Log(LogLevel::Error, message);
	EnsureSink(config.error_sink, FormatMessage(LogLevel::Error, config.module_name, message));
}

/*
=============
Log

Log a pre-formatted message if the level is enabled.
=============
*/
void Log(LogLevel level, std::string_view message)
{
	if (!IsLogLevelEnabled(level))
	return;

	const LoggerConfig config = GetLoggerConfig();
	EnsureSink(config.print_sink, FormatMessage(level, config.module_name, message));
}

/*
=============
LogLevelLabel

Provide a short string label for the supplied log level.
=============
*/
const char* LogLevelLabel(LogLevel level)
{
	switch (level) {
	case LogLevel::Trace:
		return "TRACE";
	case LogLevel::Debug:
		return "DEBUG";
	case LogLevel::Info:
		return "INFO";
	case LogLevel::Warn:
		return "WARN";
	case LogLevel::Error:
	default:
		return "ERROR";
	}
}

} // namespace worr
