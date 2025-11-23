#include "shared/logger.hpp"

#include <atomic>
#include <cassert>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
	std::mutex g_sinkMutex;
	std::vector<std::string> g_printMessages;
	std::vector<std::string> g_errorMessages;

/*
=============
CollectPrint

Store a print sink message for verification.
=============
*/
	void CollectPrint(const char* message)
{
		std::scoped_lock lock(g_sinkMutex);
		g_printMessages.emplace_back(message);
	}

/*
=============
CollectError

Store an error sink message for verification.
=============
*/
	void CollectError(const char* message)
{
		std::scoped_lock lock(g_sinkMutex);
		g_errorMessages.emplace_back(message);
	}

/*
=============
ToggleLevels

Repeatedly adjust the log level to exercise atomic coordination.
=============
*/
	void ToggleLevels()
{
		for (int i = 0; i < 200; ++i) {
			worr::SetLogLevel((i % 2) == 0 ? worr::LogLevel::Info : worr::LogLevel::Trace);
	}
	}

/*
=============
LogMessages

Emit informational and error messages while configuration changes.
=============
*/
	void LogMessages()
{
		for (int i = 0; i < 200; ++i) {
			worr::Log(worr::LogLevel::Info, "concurrent-info");
			worr::Log(worr::LogLevel::Error, "concurrent-error");
	}
	}
} // namespace

/*
=============
main

Verify concurrent logging preserves configuration integrity.
=============
*/
int main()
{
	worr::InitLogger("threaded", &CollectPrint, &CollectError);

	std::thread levelThread(&ToggleLevels);
	std::thread logThreadA(&LogMessages);
	std::thread logThreadB(&LogMessages);

	levelThread.join();
	logThreadA.join();
	logThreadB.join();

	std::scoped_lock lock(g_sinkMutex);
	assert(!g_printMessages.empty());
	assert(!g_errorMessages.empty());

	const std::string prefix = "[WORR][threaded]";
	for (const std::string& message : g_printMessages) {
		assert(message.rfind(prefix, 0) == 0);
	}
	for (const std::string& message : g_errorMessages) {
		assert(message.rfind(prefix, 0) == 0);
	}

	return 0;
}
