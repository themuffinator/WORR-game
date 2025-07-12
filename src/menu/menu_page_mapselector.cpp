#include "../g_local.h"

static void OpenMapSelectorMenu(gentity_t *ent) {
	if (!ent || !ent->client) return;

	MenuSystem::Open(ent, MenuBuilder()
		.add(hostname->string, MenuAlign::Center)
		.spacer().spacer().spacer().spacer().spacer()
		.add("Vote for the next arena:", MenuAlign::Center)
		.spacer()
		.add("", MenuAlign::Center)
		.add("", MenuAlign::Center)
		.add("", MenuAlign::Center)
		.spacer().spacer().spacer().spacer().spacer().spacer().spacer()
		.add("", MenuAlign::Left)
		.update([](gentity_t *ent, const Menu &m) {
			auto &menu = const_cast<Menu &>(m);
			const int totalBars = 24;
			float elapsed = (level.time - level.mapSelectorVoteStartTime).seconds();
			elapsed = std::clamp(elapsed, 0.0f, 5.0f);

			int filled = static_cast<int>((elapsed / 10.0f) * totalBars);
			int empty = totalBars - filled;
			std::string bar = std::string(empty, '>');

			menu.entries[0].text = hostname->string;
			menu.entries[6].text = "Vote for the next arena:";

			for (int i = 0; i < 3; ++i) {
				int index = 8 + i;
				auto *candidate = level.mapSelectorVoteCandidates[i];
				if (candidate) {
					menu.entries[index] = VoteEntry(
						candidate->longName.empty() ? candidate->filename : candidate->longName,
						i
					);
				} else {
					menu.entries[index].text.clear();
					menu.entries[index].onSelect = nullptr;
				}
			}

			menu.entries[17].text = bar;
			})
		.build());
}
