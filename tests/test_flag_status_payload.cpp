/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_flag_status_payload.cpp implementation.*/

#include <array>
#include <cassert>
#include <cmath>
#include <string>

#include "server/gameplay/g_capture.hpp"

std::string BuildFlagStatusPayload(bool captureTheFlagMode, const std::array<FlagStatus, 3>& statuses);

using std::sinf;

/*
=============
main

Validates that the flag payload helper encodes two- and three-character states correctly.
=============
*/
int main() {
	const std::array<FlagStatus, 3> states{
		FlagStatus::Taken,
		FlagStatus::Dropped,
		FlagStatus::AtBase
	};

	const std::string ctfPayload = BuildFlagStatusPayload(true, states);
	assert(ctfPayload.size() == 2);
	assert(ctfPayload == "12");

	const std::string oneFlagPayload = BuildFlagStatusPayload(false, states);
	assert(oneFlagPayload.size() == 3);
	assert(oneFlagPayload == "140");

	return 0;
}
