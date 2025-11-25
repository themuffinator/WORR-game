/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_map_config_filename_sanitization.cpp implementation.*/

#include "shared/map_validation.hpp"

#include <cassert>
#include <string>

/*
=============
main

Exercises map configuration filename sanitization boundary and failure cases.
=============
*/
int main()
{
	std::string sanitized;
	std::string reason;

	// Valid entry trims whitespace and preserves extensions.
	reason.clear();
	assert(G_SanitizeMapConfigFilename("  arena.cfg  ", sanitized, reason));
	assert(sanitized == "arena.cfg");
	assert(reason.empty());

	// Reject empty or whitespace-only input.
	sanitized = "placeholder";
	reason.clear();
	assert(!G_SanitizeMapConfigFilename("   \t\n", sanitized, reason));
	assert(sanitized.empty());
	assert(reason == "is empty");

	// Reject absolute paths.
	sanitized = "placeholder";
	reason.clear();
	assert(!G_SanitizeMapConfigFilename("/maps/duel.cfg", sanitized, reason));
	assert(reason == "is an absolute path");

	// Block traversal tokens before path separators are considered.
	sanitized = "placeholder";
	reason.clear();
	assert(!G_SanitizeMapConfigFilename("maps/..", sanitized, reason));
	assert(reason == "contains traversal tokens");

	// Disallow nested directories via path separators.
	sanitized = "placeholder";
	reason.clear();
	assert(!G_SanitizeMapConfigFilename("maps/config.cfg", sanitized, reason));
	assert(reason == "contains path separators");

	// Catch device specifiers.
	sanitized = "placeholder";
	reason.clear();
	assert(!G_SanitizeMapConfigFilename("C:arena.cfg", sanitized, reason));
	assert(reason == "contains a device specifier");

	// Flag other illegal characters.
	sanitized = "placeholder";
	reason.clear();
	assert(!G_SanitizeMapConfigFilename("bad?name.cfg", sanitized, reason));
	assert(reason == "contains illegal characters");

	// Allow dotted names that match expected character sets.
	reason.clear();
	assert(G_SanitizeMapConfigFilename("subarena.v1.cfg", sanitized, reason));
	assert(sanitized == "subarena.v1.cfg");
	assert(reason.empty());

	return 0;
}
