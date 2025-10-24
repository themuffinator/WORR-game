#pragma once

#include <cstdint>
#include <span>
#include <string_view>

struct gentity_t;

namespace Commands {

struct VoteDefinitionView {
        std::string_view name;
        int32_t flag;
        bool visibleInMenu;
};

bool TryLaunchVote(gentity_t* ent, std::string_view voteName, std::string_view voteArg);
std::span<const VoteDefinitionView> GetRegisteredVoteDefinitions();

} // namespace Commands

