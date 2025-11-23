#pragma once

#include <format>
#include <functional>
#include <string_view>

namespace worr {
	enum class LogLevel {
		Trace = 0,
		Debug,
		Info,
		Warn,
		Error
	};

	/*
	=============
	InitLogger

	Initialize the logger with module metadata and output sinks.
	=============
	*/
void InitLogger(std::string_view module_name, std::function<void(std::string_view)> print_sink, std::function<void(std::string_view)> error_sink);

	/*
	=============
	SetLogLevel

	Override the current logging level programmatically.
	=============
	*/
	void SetLogLevel(LogLevel level);

	/*
	=============
	GetLogLevel

	Fetch the currently active log level.
	=============
	*/
	LogLevel GetLogLevel();

	/*
	=============
	IsLogLevelEnabled

	Return whether the provided log level should emit output.
	=============
	*/
	bool IsLogLevelEnabled(LogLevel level);

	/*
	=============
	LoggerPrint

	Hook-compatible printer that respects the configured log level.
	=============
	*/
	void LoggerPrint(const char* message);

	/*
	=============
	LoggerError

	Hook-compatible error printer that always emits output.
	=============
	*/
	void LoggerError(const char* message);

	/*
	=============
	Log

	Log a pre-formatted message if the level is enabled.
	=============
	*/
	void Log(LogLevel level, std::string_view message);

	/*
	=============
	Logf

	Format a message and log it if the level is enabled.
	=============
	*/
	template<typename... Args>
	inline void Logf(LogLevel level, std::format_string<Args...> format_str, Args &&... args)
	{
		if (!IsLogLevelEnabled(level))
			return;

		Log(level, std::format(format_str, std::forward<Args>(args)...));
	}

	/*
	=============
	LogLevelLabel

	Provide a short string label for the supplied log level.
	=============
	*/
	const char* LogLevelLabel(LogLevel level);

} // namespace worr
