#include "shared/map_validation.hpp"

#include <json/json.h>

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

/*
=============
main

Validates map pool sanitization using crafted JSON payloads.
=============
*/
int main()
{
	const std::string json = R"JSON(
{
	"maps": [
		{ "bsp": "q2dm1", "dm": true },
		{ "bsp": "  q2dm2  ", "dm": true },
		{ "bsp": "bad/idea", "dm": true },
		{ "bsp": "..\\sneaky", "dm": true }
	]
}
)JSON";

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;
	std::istringstream stream(json);
	const bool parsed = Json::parseFromStream(builder, stream, &root, &errs);
	assert(parsed);
	assert(errs.empty());
	assert(root.isMember("maps"));
	assert(root["maps"].isArray());

	std::vector<std::string> accepted;
	std::vector<std::string> rejectedReasons;
	std::string reason;

	for (const auto& entry : root["maps"]) {
		const std::string bspName = entry["bsp"].asString();
		std::string sanitized;
		if (G_SanitizeMapPoolFilename(bspName, sanitized, reason)) {
			assert(reason.empty());
			accepted.push_back(sanitized);
		}
		else {
			assert(!reason.empty());
			rejectedReasons.push_back(reason);
		}
	}

	assert(accepted.size() == 2);
	assert(accepted[0] == "q2dm1");
	assert(accepted[1] == "q2dm2");

	assert(rejectedReasons.size() == 2);
	assert(rejectedReasons[0] == "contains path separators");
	assert(rejectedReasons[1] == "contains traversal tokens");

	return 0;
}
