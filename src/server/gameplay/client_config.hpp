#pragma once

#include <string>

struct gclient_t;
struct Ghosts;

void ClientConfig_Init(gclient_t* cl, const std::string& playerID, const std::string& playerName, const std::string& gameType);
void ClientConfig_SaveStats(gclient_t* cl, bool wonMatch);
void ClientConfig_SaveStatsForGhost(const Ghosts& ghost, bool won);
void ClientConfig_SaveWeaponPreferences(gclient_t* cl);
int ClientConfig_DefaultSkillRating();

