#pragma once

#include <cstdint>

struct GameTime;
struct gclient_t;
struct gentity_t;

enum class GameType : std::uint8_t;

#include "server/match/match_state_helper.hpp"
#include "server/match/match_state_utils.hpp"

void Round_End();
void Match_Start();
void Match_End();
void Match_Reset();
void Gauntlet_RemoveLoser();
void Gauntlet_MatchEnd_AdjustScores();
void ReadyAll();
void UnReadyAll();
void AnnounceCountdown(int t, GameTime& checkRef);
void CheckDMExitRules();
void CheckDMEndFrame();
void CheckVote();
void ChangeGametype(GameType gt);
int GT_ScoreLimit();
const char* GT_ScoreLimitString();
void GT_Init();
void MatchStats_Init();
void MatchStats_End();
