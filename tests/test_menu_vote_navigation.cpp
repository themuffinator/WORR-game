#include <cmath>
#include <memory>

namespace std {
        using ::sinf;
}

#include "g_local.hpp"

#include <cassert>

// Provide stubs required by menu_main.cpp without pulling in the full game.
local_game_import_t gi{};

int main() {
        // Menus without any actionable entries should leave the cursor untouched.
        Menu blankMenu;
        blankMenu.entries.emplace_back("", MenuAlign::Left);
        blankMenu.entries.emplace_back("", MenuAlign::Left);
        blankMenu.entries.emplace_back("", MenuAlign::Left);
        blankMenu.current = 1;
        blankMenu.Next();
        assert(blankMenu.current == 1);
        blankMenu.Prev();
        assert(blankMenu.current == 1);

        // Starting from an invalid cursor must normalize before scanning so loops terminate.
        Menu invalidStart = blankMenu;
        invalidStart.current = -1;
        invalidStart.Next();
        assert(invalidStart.current == -1);
        invalidStart.Prev();
        assert(invalidStart.current == -1);

        // Simulate the vote menu during the "GET READY TO VOTE!" countdown: all entries are inert.
        Menu voteCountdown;
        for (int i = 0; i < MAX_VISIBLE_LINES; ++i)
                voteCountdown.entries.emplace_back("", MenuAlign::Center);
        voteCountdown.current = -1;
        voteCountdown.Next();
        assert(voteCountdown.current == -1);
        voteCountdown.Prev();
        assert(voteCountdown.current == -1);

        // Once the Yes/No handlers are available, navigation should cycle between them.
        Menu voteLive = voteCountdown;
        voteLive.entries[7].text = "[ YES ]";
        voteLive.entries[7].onSelect = [](gentity_t *, Menu &) {};
        voteLive.entries[8].text = "[ NO ]";
        voteLive.entries[8].onSelect = [](gentity_t *, Menu &) {};

        voteLive.current = -1;
        voteLive.Next();
        assert(voteLive.current == 7);
        voteLive.Next();
        assert(voteLive.current == 8);
        voteLive.Next();
        assert(voteLive.current == 7);

        voteLive.current = 8;
        voteLive.Prev();
        assert(voteLive.current == 7);
        voteLive.Prev();
        assert(voteLive.current == 8);

        // If only one actionable entry exists, navigation should remain on that entry.
        Menu singleAction;
        singleAction.entries.emplace_back("", MenuAlign::Left);
        singleAction.entries.emplace_back("", MenuAlign::Left);
        singleAction.entries.emplace_back("", MenuAlign::Left);
        singleAction.entries[1].onSelect = [](gentity_t *, Menu &) {};
        singleAction.current = 1;
        singleAction.Next();
        assert(singleAction.current == 1);
        singleAction.Prev();
        assert(singleAction.current == 1);

        return 0;
}
