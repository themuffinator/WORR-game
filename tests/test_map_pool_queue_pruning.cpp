/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_map_pool_queue_pruning.cpp implementation.*/

#include "server/g_local.hpp"

#include <cassert>
#include <string>
#include <vector>

/*
=============
main

Validates that queued map requests referencing maps removed from the pool are
cleared when the pool is refreshed.
=============
*/
int main()
{
	MapSystem system{};

	MapEntry existing{};
	existing.filename = "q2dm1";

	system.mapPool.push_back(existing);
	system.EnqueueMyMapRequest(existing, "PlayerOne", 0, 0, GameTime::from_sec(15));
	assert(system.playQueue.size() == 1);
	assert(system.myMapQueue.size() == 1);

	MapEntry newEntry{};
	newEntry.filename = "q2dm2";
	system.mapPool.clear();
	system.mapPool.push_back(newEntry);

	std::vector<std::string> removed;
	system.PruneQueuesToMapPool(&removed);

	assert(system.playQueue.empty());
	assert(system.myMapQueue.empty());
	assert(!removed.empty());

	return 0;
}
