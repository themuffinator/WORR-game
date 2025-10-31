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
};

extern void OpenJoinMenu(gentity_t* ent);

void OpenAdminSettingsMenu(gentity_t* ent) {
	auto settings = std::make_shared<AdminSettings>();
	static std::vector<std::string> rulesetNames = { "Casual", "Standard", "Competitive" };
	static int rulesetIndex = 1; // start at "Standard"

	MenuBuilder builder;
	builder.add("*Settings Menu", MenuAlign::Center)
		.spacer();

	{
		auto [text, align, cb] = MakeCycle([settings]() { return settings->timeLimit; }, [settings]() {
			settings->timeLimit = (settings->timeLimit + 5) % 60;
			if (settings->timeLimit < 5) settings->timeLimit = 5;
			});
		builder.add(text, align, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->weaponsstay; }, [settings]() { settings->weaponsstay = !settings->weaponsstay; });
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->instantItems; }, [settings]() { settings->instantItems = !settings->instantItems; });
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->pu_drop; }, [settings]() { settings->pu_drop = !settings->pu_drop; });
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->instantweap; }, [settings]() { settings->instantweap = !settings->instantweap; });
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeToggle([settings]() { return settings->match_lock; }, [settings]() { settings->match_lock = !settings->match_lock; });
		builder.add("", MenuAlign::Left, cb);
	}

	{
		auto [_, __, cb] = MakeChoice(rulesetNames, []() { return rulesetIndex; }, []() {
			rulesetIndex = (rulesetIndex + 1) % 3;
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
		menu.entries[i++].text = fmt::format("time limit: {:2} mins", settings->timeLimit);
		menu.entries[i++].text = fmt::format("weapons stay: {}", settings->weaponsstay ? "Yes" : "No");
		menu.entries[i++].text = fmt::format("instant items: {}", settings->instantItems ? "Yes" : "No");
		menu.entries[i++].text = fmt::format("powerup drops: {}", settings->pu_drop ? "Yes" : "No");
		menu.entries[i++].text = fmt::format("instant weapon switch: {}", settings->instantweap ? "Yes" : "No");
		menu.entries[i++].text = fmt::format("match lock: {}", settings->match_lock ? "Yes" : "No");
		menu.entries[i++].text = fmt::format("ruleset: {}", rulesetNames[rulesetIndex]);
			});

	MenuSystem::Open(ent, builder.build());
}
