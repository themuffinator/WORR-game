// Copyright (c) DarkMatter Projects 2023-2025
// Licensed under the GNU General Public License 2.0.
//
// command_system.hpp - Public interface for the modernized command system.

#pragma once

#include "server/g_local.hpp"
#include <string_view>
#include <optional>
#include <functional>
#include <charconv>
#include <string>
#include <vector>
#include <initializer_list>

// C++20 Heterogeneous Lookup Support for string_view
// Allows searching maps with string_view without creating a std::string.
struct StringViewHash {
	using is_transparent = void;
	[[nodiscard]] size_t operator()(const char* txt) const { return std::hash<std::string_view>{}(txt); }
	[[nodiscard]] size_t operator()(std::string_view txt) const { return std::hash<std::string_view>{}(txt); }
	[[nodiscard]] size_t operator()(const std::string& txt) const { return std::hash<std::string>{}(txt); }
};

enum class CommandFlag : uint32_t {
	None = 0,
	AllowDead = 1 << 0,
	AllowIntermission = 1 << 1,
	AllowSpectator = 1 << 2,
	MatchOnly = 1 << 3,
	AdminOnly = 1 << 4,
	CheatProtect = 1 << 5,
};
template<> struct is_bitmask_enum<CommandFlag> : std::true_type {};

class CommandArgs {
private:
	int _argc;
	std::vector<std::string> _manualArgs;
	bool _useManualArgs = false;

	template<typename It>
	void setManualArgs(It begin, It end) {
		_manualArgs.clear();
		for (auto it = begin; it != end; ++it) {
			_manualArgs.emplace_back(*it);
		}
		_argc = static_cast<int>(_manualArgs.size());
		_useManualArgs = true;
	}

public:
	CommandArgs() : _argc(gi.argc()) {}

	CommandArgs(std::initializer_list<std::string_view> args) {
		setManualArgs(args.begin(), args.end());
	}

	explicit CommandArgs(std::vector<std::string> args)
		: _argc(static_cast<int>(args.size())),
		_manualArgs(std::move(args)),
		_useManualArgs(true) {
	}

	int count() const { return _argc; }

	std::string_view getString(int index) const {
		if (index < 0 || index >= _argc) return "";
		if (_useManualArgs) {
			return _manualArgs[index];
		}
		return gi.argv(index);
	}

	std::string joinFrom(int index) const {
		if (index < 0 || index >= _argc) {
			return {};
		}

		std::string result;
		auto append = [&result](std::string_view part) {
			if (part.empty()) {
				return;
			}
			if (!result.empty()) {
				result.push_back(' ');
			}
			result.append(part.data(), part.size());
			};

		if (_useManualArgs) {
			for (int i = index; i < _argc; ++i) {
				append(_manualArgs[i]);
			}
		}
		else {
			for (int i = index; i < _argc; ++i) {
				append(gi.argv(i));
			}
		}

		return result;
	}

	std::optional<int> getInt(int index) const {
		return ParseInt(getString(index));
	}

	static std::optional<int> ParseInt(std::string_view str) {
		int value;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
		if (ec == std::errc() && ptr == str.data() + str.size()) {
			return value;
		}
		return std::nullopt;
	}

	std::optional<float> getFloat(int index) const {
		return ParseFloat(getString(index));
	}

	static std::optional<float> ParseFloat(std::string_view str) {
		float value;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
		if (ec == std::errc() && ptr == str.data() + str.size()) {
			return value;
		}
		return std::nullopt;
	}
};

struct Command {
	std::function<void(gentity_t*, const CommandArgs&)> function;
	BitFlags<CommandFlag> flags;
	bool floodExempt = false;
};

struct VoteCommand {
	std::string_view name;
	std::function<bool(gentity_t*, const CommandArgs&)> validate; // Corrected signature
	std::function<void()> execute;
	int32_t flag = 0;
	int8_t minArgs = 1;
	std::string_view argsUsage;
	std::string_view helpText;

	// Added constructor to fix initialization errors
	VoteCommand(std::string_view name,
		std::function<bool(gentity_t*, const CommandArgs&)> validate,
		std::function<void()> execute,
		int32_t flag,
		int8_t minArgs,
		std::string_view argsUsage,
		std::string_view helpText)
		: name(name), validate(std::move(validate)), execute(std::move(execute)),
		flag(flag), minArgs(minArgs), argsUsage(argsUsage), helpText(helpText) {
	}

	// Default constructor for map compatibility
	VoteCommand() = default;
};

// Main dispatcher function called by the engine.
void ClientCommand(gentity_t* ent);

bool CheckFlood(gentity_t* ent);

// Main registration function to be called once at game startup.
void RegisterAllCommands();

inline bool CheatsOk(gentity_t* ent);
