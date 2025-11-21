#pragma once

#include <cctype>
#include <string>
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
G_SanitizeMapPoolFilename

Trims whitespace and rejects any map entry filenames containing
path separators, traversal tokens, or other illegal characters.
=============
*/
inline bool G_SanitizeMapPoolFilename(std::string_view rawName, std::string& sanitized, std::string& rejectReason)
	{
	const size_t start = rawName.find_first_not_of(" \t\r\n");
	if (start == std::string_view::npos) {
	sanitized.clear();
	rejectReason = "is empty";
	return false;
	}

	const size_t end = rawName.find_last_not_of(" \t\r\n");
	std::string candidate(rawName.substr(start, end - start + 1));

	if (candidate.find('/') != std::string::npos || candidate.find('\\') != std::string::npos) {
	rejectReason = "contains path separators";
	return false;
	}

	if (candidate == "." || candidate == ".." || candidate.find("..") != std::string::npos) {
	rejectReason = "contains traversal tokens";
	return false;
	}

	if (!G_IsValidMapIdentifier(candidate)) {
	rejectReason = "contains illegal characters";
	return false;
	}

	sanitized = std::move(candidate);
	rejectReason.clear();
	return true;
}

/*
=============
G_SanitizeMapConfigFilename

Trims whitespace and rejects map configuration filenames containing
absolute paths, traversal tokens, or disallowed characters. Permits
periods for extensions.
=============
*/
inline bool G_SanitizeMapConfigFilename(std::string_view rawName, std::string& sanitized, std::string& rejectReason)
{
const size_t start = rawName.find_first_not_of(" \t\r\n");
	if (start == std::string_view::npos) {
		sanitized.clear();
		rejectReason = "is empty";
		return false;
	}

const size_t end = rawName.find_last_not_of(" \t\r\n");
	std::string candidate(rawName.substr(start, end - start + 1));

if (candidate.front() == '/' || candidate.front() == '\\') {
		rejectReason = "is an absolute path";
		return false;
	}

	if (candidate.find("..") != std::string::npos) {
		rejectReason = "contains traversal tokens";
		return false;
	}

if (candidate.find('/') != std::string::npos || candidate.find('\\') != std::string::npos) {
		rejectReason = "contains path separators";
		return false;
	}

	if (candidate.find(':') != std::string::npos) {
		rejectReason = "contains a device specifier";
		return false;
	}

	for (char ch : candidate) {
		const unsigned char uch = static_cast<unsigned char>(ch);
		if (std::isalnum(uch))
			continue;

		switch (ch) {
		case '_':
		case '-':
		case '.':
			continue;
		default:
			rejectReason = "contains illegal characters";
			return false;
		}
	}

	sanitized = std::move(candidate);
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
