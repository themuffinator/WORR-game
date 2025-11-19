#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

void SanitizeSpectatorString(const char* in, char* out, size_t outSize);

struct MapRecord {
	std::string filename;
	std::string longName;
};

/*
=============
str_contains_case

Case-insensitive substring helper for tests
=============
*/
static bool str_contains_case(const std::string& haystack, const std::string& needle) {
	return std::search(
		haystack.begin(), haystack.end(),
		needle.begin(), needle.end(),
		[](char ch1, char ch2) {
			return std::tolower(static_cast<unsigned char>(ch1)) == std::tolower(static_cast<unsigned char>(ch2));
		}
	) != haystack.end();
}

/*
=============
main

Unicode filtering regression verification
=============
*/
int main() {
	const std::vector<MapRecord> maps = {
		{ "dm_plain", "Plain Arena" },
		{ "dm_fjord", "Fjörd Arena" },
		{ std::string("dm_") + std::string(1, static_cast<char>(0xFF)) + "_vault", std::string("Citadel ") + std::string(1, static_cast<char>(0xFF)) }
	};

	std::vector<std::string> filtered;
	const std::string filterNeedle = "fjörd";
	for (const auto& map : maps) {
		if (str_contains_case(map.filename, filterNeedle) || str_contains_case(map.longName, filterNeedle)) {
			filtered.push_back(map.filename);
		}
	}
	assert(filtered.size() == 1);
	assert(filtered.front() == "dm_fjord");

	const std::string highByteNeedle = std::string(1, static_cast<char>(0xFF));
	const bool highByteMatch = str_contains_case(maps.back().longName, highByteNeedle);
	assert(highByteMatch);
	
	char sanitizedBuffer[32] = {};
	SanitizeSpectatorString("FJ\xC3\x96RD\x1F", sanitizedBuffer, sizeof(sanitizedBuffer));
	assert(std::string(sanitizedBuffer) == "fj\xC3\x96rd");
	
	SanitizeSpectatorString("\xFFChamp\x01", sanitizedBuffer, sizeof(sanitizedBuffer));
	assert(std::string(sanitizedBuffer) == "\xFFchamp");
	
	char nearLimitBuffer[8];
	std::fill(std::begin(nearLimitBuffer), std::end(nearLimitBuffer), 'x');
	SanitizeSpectatorString("123456789", nearLimitBuffer, sizeof(nearLimitBuffer));
	assert(std::string(nearLimitBuffer) == "1234567");
	assert(nearLimitBuffer[7] == '\0');

	return 0;
}
