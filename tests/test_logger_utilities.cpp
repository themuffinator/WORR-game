/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_logger_utilities.cpp implementation.*/

#include "shared/logger.hpp"

#include <cassert>
#include <cstdlib>
#include <string>
#include <string_view>

using namespace worr;

/*
=============
main

Validate logger utilities parsing, environment handling, ordering, and formatting.
=============
*/
int main()
{
	// ParseLogLevel accepts various valid inputs and defaults to Info for unknown values.
	assert(ParseLogLevel("TRACE") == LogLevel::Trace);
	assert(ParseLogLevel("debug") == LogLevel::Debug);
	assert(ParseLogLevel("Warn") == LogLevel::Warn);
	assert(ParseLogLevel("warning") == LogLevel::Warn);
	assert(ParseLogLevel("error") == LogLevel::Error);
	assert(ParseLogLevel("anything-else") == LogLevel::Info);

	// ReadLogLevelFromEnv respects unset, valid, and invalid environment values.
	unsetenv("WORR_LOG_LEVEL");
	assert(ReadLogLevelFromEnv() == LogLevel::Info);
	setenv("WORR_LOG_LEVEL", "DEBUG", 1);
	assert(ReadLogLevelFromEnv() == LogLevel::Debug);
	setenv("WORR_LOG_LEVEL", "unknown", 1);
	assert(ReadLogLevelFromEnv() == LogLevel::Info);
	unsetenv("WORR_LOG_LEVEL");

	// LevelWeight enforces strict ordering from trace through error.
	assert(LevelWeight(LogLevel::Trace) < LevelWeight(LogLevel::Debug));
	assert(LevelWeight(LogLevel::Debug) < LevelWeight(LogLevel::Info));
	assert(LevelWeight(LogLevel::Info) < LevelWeight(LogLevel::Warn));
	assert(LevelWeight(LogLevel::Warn) < LevelWeight(LogLevel::Error));

	// FormatMessage prefixes module and level, appending a trailing newline when needed.
	const std::string debug_message = FormatMessage(LogLevel::Debug, "mod", "hello");
	assert(debug_message == "[WORR][mod] [DEBUG] hello\n");

	const std::string warned = FormatMessage(LogLevel::Warn, "mod", "already newline\n");
	assert(warned == "[WORR][mod] [WARN] already newline\n");

	return 0;
}
