#pragma once

#include "../g_local.hpp"
#include "../../shared/char_array_utils.hpp"

/*
=============
ValidateEntityCapacityOrError

Ensures the configured entity limit can host the requested client count
plus the BODY_QUEUE entries and the world entity.
=============
*/
inline void ValidateEntityCapacityOrError(int maxEntities, int maxClients) {
	const int requiredEntities = maxClients + static_cast<int>(BODY_QUEUE_SIZE) + 1;

	if (maxEntities < requiredEntities) {
		gi.Com_Error(G_Fmt(
			"InitGame: maxentities ({}) is too small for {} maxclients; requires at least {} entities ({} clients + {} BODY_QUEUE + 1)",
			maxEntities,
			maxClients,
			requiredEntities,
			maxClients,
			BODY_QUEUE_SIZE).data());
	}
}
