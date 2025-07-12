#include "../g_local.h"

struct AdminSettings {
	int timelimit = 15;
	bool weaponsstay = false;
	bool instantItems = false;
	bool pu_drop = false;
	bool instantweap = false;
	bool match_lock = false;
};

extern void OpenJoinMenu(gentity_t *ent);

void OpenAdminSettingsMenu(gentity_t *ent) {
	auto settings = std::make_shared<AdminSettings>();
	static std::vector<std::string> rulesetNames = { "Casual", "Standard", "Competitive" };
	static int rulesetIndex = 1; // start at "Standard"

	MenuBuilder builder;
	builder.add("*Settings Menu", MenuAlign::Center)
		.spacer();

	{
		auto [text, align, cb] = MakeCycle([settings]() { return settings->timelimit; }, [settings]() {
			settings->timelimit = (settings->timelimit + 5) % 60;
			if (settings->timelimit < 5) settings->timelimit = 5;
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
		.add("Return", MenuAlign::Left, [](gentity_t *ent, Menu &) {
		OpenJoinMenu(ent);
			})
		.context(settings)
				.update([settings](gentity_t *ent, const Menu &m) {
					auto &menu = const_cast<Menu &>(m);
					int i = 2;
					menu.entries[i++].text = fmt::format("time limit: {:2} mins", settings->timelimit);
					menu.entries[i++].text = fmt::format("weapons stay: {}", settings->weaponsstay ? "Yes" : "No");
					menu.entries[i++].text = fmt::format("instant items: {}", settings->instantItems ? "Yes" : "No");
					menu.entries[i++].text = fmt::format("powerup drops: {}", settings->pu_drop ? "Yes" : "No");
					menu.entries[i++].text = fmt::format("instant weapon switch: {}", settings->instantweap ? "Yes" : "No");
					menu.entries[i++].text = fmt::format("match lock: {}", settings->match_lock ? "Yes" : "No");
					menu.entries[i++].text = fmt::format("ruleset: {}", rulesetNames[rulesetIndex]);
					});

			MenuSystem::Open(ent, builder.build());
}
