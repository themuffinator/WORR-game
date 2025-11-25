/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_logger_concurrency.cpp implementation.*/

#include "shared/logger.hpp"

#include <cassert>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace worr;

/*
=============
main

Validate concurrent logging access keeps configuration intact.
=============
*/
int main()
{
	std::mutex sink_mutex;
	std::vector<std::string> printed;
	std::vector<std::string> errors;

	InitLogger("threaded", [&](std::string_view message) {
		std::lock_guard lock(sink_mutex);
		printed.emplace_back(message);
	}, [&](std::string_view message) {
		std::lock_guard lock(sink_mutex);
		errors.emplace_back(message);
	});

	SetLogLevel(LogLevel::Debug);

	constexpr int kThreads = 8;
	constexpr int kIterations = 50;

	std::atomic<bool> running{ true };
	std::thread toggler([&]() {
		for (int i = 0; i < kThreads * kIterations; ++i) {
			SetLogLevel((i % 2 == 0) ? LogLevel::Debug : LogLevel::Info);
		}
		running = false;
	});

	std::vector<std::thread> workers;
	workers.reserve(kThreads);
	for (int i = 0; i < kThreads; ++i) {
		workers.emplace_back([&]() {
			for (int j = 0; j < kIterations; ++j)
				Log(LogLevel::Info, "ping");
		});
	}

	for (auto& worker : workers)
		worker.join();

	toggler.join();
	assert(!running.load());
	assert(static_cast<int>(printed.size()) == kThreads * kIterations);

	for (const auto& message : printed)
		assert(message.find("[WORR][threaded]") != std::string::npos);

	for (const auto& message : errors)
		assert(message.find("[WORR][threaded]") != std::string::npos);

	return 0;
}
