#include <array>
#include <cassert>
#include <string>

#define SPECTATOR_SANITIZE_ONLY
#include "../src/server/gameplay/g_spectator.cpp"
#undef SPECTATOR_SANITIZE_ONLY

/*
=============
main

Verifies spectator name sanitization lowercases ASCII characters, removes control codes, and preserves non-ASCII bytes.
=============
*/
int main()
{
	std::array<char, 64> buffer{};

	auto verify = [&buffer](const char* input, const std::string& expected) {
		buffer.fill('\0');
		SanitizeString(input, buffer.data());
		assert(std::string(buffer.data()) == expected);
	};

	const char controlName[] = { 'R', '\x01', 'a', '\x02', 'W', '\0' };
	verify(controlName, "raw");
	verify("Señor_Ω", "señor_Ω");
	verify("ÄLpha\tβ", "Älphaβ");

	return 0;
}
