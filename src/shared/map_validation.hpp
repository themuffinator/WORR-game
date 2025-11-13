#pragma once

#include <cctype>
#include <string_view>

/*
=============
G_IsValidMapIdentifier

Returns true when the provided map identifier only contains expected
characters and lacks any traversal tokens.
=============
*/
inline bool G_IsValidMapIdentifier(std::string_view mapName)
{
	if (mapName.empty())
		return false;

	if (mapName == "." || mapName == "..")
		return false;

	if (mapName.find("..") != std::string_view::npos)
		return false;

	for (char ch : mapName) {
		const unsigned char uch = static_cast<unsigned char>(ch);
		if (std::isalnum(uch))
			continue;

		switch (ch) {
		case '_':
		case '-':
			continue;
		default:
			return false;
		}
	}

	return true;
}

/*
=============
G_IsValidOverrideDirectory

Validates the override directory string to prevent traversal and ensure only
expected characters are present.
=============
*/
inline bool G_IsValidOverrideDirectory(std::string_view directory)
{
	if (directory.empty())
		return false;

	if (directory.front() == '/' || directory.front() == '\\')
		return false;

	if (directory.find("..") != std::string_view::npos)
		return false;

	if (directory.find('\\') != std::string_view::npos)
		return false;

	if (directory.find(':') != std::string_view::npos)
		return false;

	size_t pos = 0;
	while (pos < directory.size()) {
		const size_t next = directory.find('/', pos);
		std::string_view segment = directory.substr(pos, next - pos);

		if (segment.empty())
			return false;

		if (segment == "." || segment == "..")
			return false;

		for (char ch : segment) {
			const unsigned char uch = static_cast<unsigned char>(ch);
			if (std::isalnum(uch))
				continue;

			switch (ch) {
			case '_':
			case '-':
				continue;
			default:
				return false;
			}
		}

		if (next == std::string_view::npos)
			break;

		pos = next + 1;
	}

	return true;
}
