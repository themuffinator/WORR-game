// menu_page_mapselector.cpp (Menu Page - Map Selector)
// This file implements the end-of-match map voting screen. This is a critical
// part of the multiplayer flow, allowing players to choose the next map to
// be played from a list of randomly selected candidates.
//
// Key Responsibilities:
// - Map Candidate Display: The `onUpdate` function populates the menu with the
//   names of the three candidate maps chosen by the server.
// - Vote Casting: The `onSelect` callbacks for each map option call the
//   `MapSelector_CastVote` function to register the player's choice.
// - Countdown Timer: It renders a visual progress bar to show the time
//   remaining for the vote.
// - Post-Vote State: After a player has voted, the menu updates to show an
//   acknowledgment message, preventing them from voting again.

#include "../g_local.hpp"

void MapSelector_CastVote(gentity_t* ent, int voteIndex);

/*
========================
OpenMapSelectorMenu
========================
*/
void OpenMapSelectorMenu(gentity_t* ent) {
	if (!ent || !ent->client)
		return;

	constexpr int NUM_CANDIDATES = 3;
	constexpr int TOTAL_BAR_SEGMENTS = 28;	//24;
	constexpr GameTime VOTE_DURATION = 5_sec;

	auto& ms = level.mapSelector;
	MenuBuilder builder;

	// --- Initial spacing ---
	builder.spacer().spacer();

	// --- Header ---
	const int headerIndex = builder.size();
	builder.add("Vote for the next arena:", MenuAlign::Center);
	builder.spacer();

	// --- Map vote entries ---
	std::array<int, NUM_CANDIDATES> voteEntryIndices{};
	for (int i = 0; i < NUM_CANDIDATES; ++i) {
		voteEntryIndices[i] = builder.size();
		builder.add("(loading...)", MenuAlign::Center, [i](gentity_t* ent, Menu& menu) {
			MapSelector_CastVote(ent, i);
			});
	}

	builder.spacer().spacer();

	// --- Acknowledgement lines ---
	const int ackIndex = builder.size();
	builder.add("", MenuAlign::Center);
	builder.add("", MenuAlign::Center);

	// --- Progress bar line ---
	const int barIndex = builder.size();
	builder.add("", MenuAlign::Center);

	// --- Update logic ---
	builder.update([=](gentity_t* ent, const Menu& m) {
		auto& menu = const_cast<Menu&>(m);
		const int clientNum = ent->s.number;
		const int vote = ms.votes[clientNum];
		const bool hasVoted = (vote >= 0 && vote < NUM_CANDIDATES && ms.candidates[vote]);

		if (!hasVoted) {
			menu.entries[headerIndex].text = "Vote for the next arena:";

			for (int i = 0; i < NUM_CANDIDATES; ++i) {
				const int idx = voteEntryIndices[i];
				auto* candidate = ms.candidates[i];

				if (candidate) {
					menu.entries[idx].text = candidate->longName.empty()
						? candidate->filename
						: candidate->longName;

					menu.entries[idx].onSelect = [i](gentity_t* ent, Menu& menu) {
						MapSelector_CastVote(ent, i);
						};
					menu.entries[idx].align = MenuAlign::Left;
				}
				else {
					menu.entries[idx].text.clear();
					menu.entries[idx].onSelect = nullptr;
				}
			}

			menu.entries[ackIndex].text.clear();
		}
		else {
			// Hide vote options, show result
			menu.entries[headerIndex].text.clear();
			for (int idx : voteEntryIndices) {
				menu.entries[idx].text.clear();
				menu.entries[idx].onSelect = nullptr;
			}

			const MapEntry* voted = ms.candidates[vote];
			const std::string name = voted->longName.empty() ? voted->filename : voted->longName;
			menu.entries[ackIndex].text = "Vote cast:";
			menu.entries[ackIndex + 1].text = G_Fmt("{}", name);
		}

		// --- Progress bar ---
		float elapsed = (level.time - ms.voteStartTime).seconds();
		elapsed = std::clamp(elapsed, 0.0f, VOTE_DURATION.seconds());

		int filled = static_cast<int>((elapsed / VOTE_DURATION.seconds()) * TOTAL_BAR_SEGMENTS);
		int empty = TOTAL_BAR_SEGMENTS - filled;

		menu.entries[barIndex].text = std::string(filled, '=') + std::string(empty, ' ');
		menu.entries[barIndex].align = MenuAlign::Left;
		});

	MenuSystem::Open(ent, builder.build());
}
