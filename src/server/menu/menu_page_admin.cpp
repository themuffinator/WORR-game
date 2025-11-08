// menu_page_admin.cpp (Menu Page - Admin)
// This file implements the administrator-specific menu pages, allowing server
// admins to manage match settings in real-time. It uses a shared context
// struct (`AdminSettings`) to temporarily store changes before they are applied.
//
// Key Responsibilities:
// - Admin Settings UI: `OpenAdminSettingsMenu` constructs the menu that allows
//   admins to toggle settings like timelimit, weapons stay, and match lock.
// - State Management: Uses a local struct (`AdminSettings`) to manage the
//   state of the menu options, which are then applied to the server's cvars
//   when the admin confirms the changes.
// - Dynamic Updates: The `onUpdate` callback ensures that the text of the menu
//   items (e.g., "weapons stay: Yes") reflects the current state of the settings
//   as the admin makes changes.

#include "../g_local.hpp"

struct AdminSettings {
	int timeLimit = 15;
	bool weaponsstay = false;
	bool instantItems = false;
	bool pu_drop = false;
	bool instantweap = false;
	bool match_lock = false;
	int playStyle = static_cast<int>(PlayStyle::Standard);
};

extern void OpenJoinMenu(gentity_t* ent);

void OpenAdminSettingsMenu(gentity_t* ent) {
	auto settings = std::make_shared<AdminSettings>();
	static const std::vector<std::string> playstyleNames = [] {
		std::vector<std::string> names;
		names.reserve(playstyle_long_name.size());
		for (auto name : playstyle_long_name)
			names.emplace_back(name);
		return names;
	}();

	if (timeLimit) {
		settings->timeLimit = static_cast<int>(timeLimit->value);
		if (settings->timeLimit < 5)
			settings->timeLimit = 5;
	}
	if (match_weaponsStay)
		settings->weaponsstay = match_weaponsStay->integer != 0;
	if (match_instantItems)
		settings->instantItems = match_instantItems->integer != 0;
	if (match_powerupDrops)
		settings->pu_drop = match_powerupDrops->integer != 0;
	if (g_instantWeaponSwitch)
		settings->instantweap = g_instantWeaponSwitch->integer != 0;
	if (match_lock)
		settings->match_lock = match_lock->integer != 0;

	int styleIndex = g_playstyle ? g_playstyle->integer : static_cast<int>(PlayStyle::Standard);
	if (styleIndex < 0 || styleIndex >= static_cast<int>(PlayStyle::Total))
		styleIndex = static_cast<int>(PlayStyle::Standard);
	settings->playStyle = styleIndex;

	MenuBuilder builder;
	builder.add("*Settings Menu", MenuAlign::Center)
		.spacer();

	{
		auto [text, align, cb] = MakeCycle([settings]() { return settings->timeLimit; }, [settings]() {
			settings->timeLimit = (settings->timeLimit + 5) % 60;
			if (settings->timeLimit < 5)
				settings->timeLimit = 5;
			gi.cvarSet("timelimit", G_Fmt("{}", settings->timeLimit).data());
		});
		builder.add(text, align, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->weaponsstay; }, [settings]() {
			settings->weaponsstay = !settings->weaponsstay;
			gi.cvarSet("match_weapons_stay", settings->weaponsstay ? "1" : "0");
		});
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->instantItems; }, [settings]() {
			settings->instantItems = !settings->instantItems;
			gi.cvarSet("match_instant_items", settings->instantItems ? "1" : "0");
		});
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->pu_drop; }, [settings]() {
			settings->pu_drop = !settings->pu_drop;
			gi.cvarSet("match_powerup_drops", settings->pu_drop ? "1" : "0");
		});
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->instantweap; }, [settings]() {
			settings->instantweap = !settings->instantweap;
			gi.cvarSet("g_instant_weapon_switch", settings->instantweap ? "1" : "0");
		});
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->match_lock; }, [settings]() {
			settings->match_lock = !settings->match_lock;
			gi.cvarSet("match_lock", settings->match_lock ? "1" : "0");
		});
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeChoice(playstyleNames, [settings]() { return settings->playStyle; }, [settings]() {
			size_t total = playstyle_long_name.size();
			settings->playStyle = (settings->playStyle + 1) % static_cast<int>(total);
			gi.cvarSet("g_playstyle", G_Fmt("{}", settings->playStyle).data());
		});
		builder.add("", MenuAlign::Left, cb);
	}

	builder.spacer().spacer().spacer().spacer().spacer().spacer().spacer()
		.add("Return", MenuAlign::Left, [](gentity_t* ent, Menu&) {
			OpenJoinMenu(ent);
		})
		.context(settings)
		.update([settings](gentity_t* ent, const Menu& m) {
			auto& menu = const_cast<Menu&>(m);
			int i = 2;
			int styleSlot = settings->playStyle;
			if (styleSlot < 0 || styleSlot >= static_cast<int>(playstyleNames.size()))
				styleSlot = static_cast<int>(PlayStyle::Standard);
			menu.entries[i++].text = fmt::format("time limit: {:2} mins", settings->timeLimit);
			menu.entries[i++].text = fmt::format("weapons stay: {}", settings->weaponsstay ? "Yes" : "No");
			menu.entries[i++].text = fmt::format("instant items: {}", settings->instantItems ? "Yes" : "No");
			menu.entries[i++].text = fmt::format("powerup drops: {}", settings->pu_drop ? "Yes" : "No");
			menu.entries[i++].text = fmt::format("instant weapon switch: {}", settings->instantweap ? "Yes" : "No");
			menu.entries[i++].text = fmt::format("match lock: {}", settings->match_lock ? "Yes" : "No");
			menu.entries[i++].text = fmt::format("play style: {}", playstyleNames[styleSlot]);
		});

	MenuSystem::Open(ent, builder.build());
}


