#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct gentity_t;
struct gclient_t;
class LevelLocals;
struct Ghosts;

enum class GameType : uint8_t;

namespace worr::server::client {

struct MatchStatsContext {
	GameType mode{};
	bool isTeamMode{};
	bool allowSkillAdjustments{};
	int redScore{};
	int blueScore{};
	std::vector<gentity_t*> participants;
	std::vector<Ghosts*> ghosts;
};

/*
=================
ClientStatsService

Coordinates skill rating adjustments and stat persistence for the current
match. The service consumes a snapshot of the match context so both
match-ending flows and disconnect flows share a single orchestration point.
=================
*/
class ClientStatsService {
public:
virtual ~ClientStatsService() = default;

	virtual void MatchStart(game_import_t& gi, GameLocals& game, LevelLocals& level) = 0;
	virtual void MatchReset(game_import_t& gi, GameLocals& game, LevelLocals& level) = 0;
	virtual void MatchEnd(game_import_t& gi, GameLocals& game, LevelLocals& level) = 0;
	virtual void RoundEnd(game_import_t& gi, GameLocals& game, LevelLocals& level) = 0;
	virtual void MarathonRegisterClientBaseline(game_import_t& gi, GameLocals& game,
		LevelLocals& level, gclient_t* client) = 0;
	virtual void UpdateDuelRecords(game_import_t& gi, GameLocals& game, LevelLocals& level) = 0;
	virtual void ChangeGametype(game_import_t& gi, GameLocals& game, LevelLocals& level,
		GameType gameType) = 0;
	virtual void CheckDMExitRules(game_import_t& gi, GameLocals& game, LevelLocals& level) = 0;
	virtual int ScoreLimit(game_import_t& gi, const GameLocals& game, const LevelLocals& level) const = 0;
virtual std::string ScoreLimitString(game_import_t& gi, const GameLocals& game,
const LevelLocals& level) const = 0;
	virtual void PersistMatchResults(const MatchStatsContext& context) = 0;
	virtual void SaveStatsForDisconnect(const MatchStatsContext& context, gentity_t* ent) = 0;
};

void InitializeClientStatsService(game_import_t& gi, GameLocals& game, LevelLocals& level);
ClientStatsService& GetClientStatsService();

} // namespace worr::server::client
