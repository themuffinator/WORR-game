#include <cmath>
#include <type_traits>

namespace std {
using ::sinf;
}

#if __cplusplus < 202302L
template<typename Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
return static_cast<std::underlying_type_t<Enum>>(e);
}

namespace std {
using ::to_underlying;
}
#endif

#include "server/g_local.hpp"
#include "server/gameplay/map_flag_parser.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

/*
=============
main

Validates MyMap queue behavior, including flag parsing, duplicate
detectors, and queue consumption ordering.
=============
*/
int main() {
	MapSystem system{};

	MapEntry first{};
	first.filename = "q2dm1";

	std::vector<std::string> flags{ "+pu", "-fd" };
	uint16_t enable = 0;
	uint16_t disable = 0;
	assert(ParseMyMapFlags(flags, enable, disable));

	const auto firstResult = system.EnqueueMyMapRequest(first, "PlayerA", enable, disable, GameTime::from_sec(5));
	assert(firstResult.accepted);
	assert(!firstResult.evictedOldest);
	assert(system.playQueue.size() == 1);
	assert(system.myMapQueue.size() == 1);
	assert(system.playQueue.front().filename == "q2dm1");
	assert(system.playQueue.front().socialID == "PlayerA");
	assert(system.playQueue.front().enableFlags == enable);
	assert(system.playQueue.front().disableFlags == disable);
	assert(system.myMapQueue.front().mapName == "q2dm1");
	assert(system.myMapQueue.front().socialID == "PlayerA");
	assert(system.myMapQueue.front().enableFlags == enable);
	assert(system.myMapQueue.front().disableFlags == disable);
	assert(system.myMapQueue.front().queuedTime == GameTime::from_sec(5));
	assert(system.IsClientInQueue("PlayerA"));
	assert(system.IsMapInQueue("q2dm1"));
	assert(!system.IsClientInQueue("PlayerB"));
	assert(!system.IsMapInQueue("q2dm3"));

	bool hasPlayerA = false;
	bool hasMap1 = false;
	for (const auto& entry : system.playQueue) {
		if (entry.socialID == "PlayerA")
			hasPlayerA = true;
		if (entry.filename == "q2dm1")
			hasMap1 = true;
	}
	assert(hasPlayerA);
	assert(hasMap1);

	MapEntry second{};
	second.filename = "q2dm3";
	const auto secondResult = system.EnqueueMyMapRequest(second, "PlayerB", 0, 0, GameTime::from_sec(10));
	assert(secondResult.accepted);
	assert(system.playQueue.size() == 2);
	assert(system.myMapQueue.size() == 2);
	assert(system.IsClientInQueue("PlayerB"));
	assert(system.IsMapInQueue("q2dm3"));

	system.ConsumeQueuedMap();

	assert(system.playQueue.size() == 1);
	assert(system.myMapQueue.size() == 1);
	assert(system.playQueue.front().filename == "q2dm3");
	assert(system.playQueue.front().socialID == "PlayerB");
	assert(system.playQueue.front().enableFlags == 0);
	assert(system.playQueue.front().disableFlags == 0);

	system.playQueue.clear();
	system.myMapQueue.clear();

	cvar_t queueLimit{};
	queueLimit.integer = 1;
	g_maps_mymap_queue_limit = &queueLimit;

	const auto limitedFirst = system.EnqueueMyMapRequest(first, "PlayerA", enable, disable, GameTime::from_sec(15));
	assert(limitedFirst.accepted);
	assert(!limitedFirst.evictedOldest);
	const auto limitedSecond = system.EnqueueMyMapRequest(second, "PlayerB", 0, 0, GameTime::from_sec(20));
	assert(limitedSecond.accepted);
	assert(limitedSecond.evictedOldest);
	assert(system.playQueue.size() == 1);
	assert(system.playQueue.front().filename == "q2dm3");
	assert(system.myMapQueue.size() == 1);
	assert(system.myMapQueue.front().socialID == "PlayerB");

	g_maps_mymap_queue_limit = nullptr;

	enable = disable = 0;
	assert(!ParseMyMapFlags({ "+unknown" }, enable, disable));

	return 0;
}
