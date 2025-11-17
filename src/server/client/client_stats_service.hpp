#pragma once

#include <cstdint>
#include <string>

struct game_import_t;
struct gclient_t;
class LevelLocals;
struct GameLocals;

enum class GameType : uint8_t;

namespace worr::server::client {

/*
=================
ClientStatsService

Defines the orchestration points for match lifecycle logic currently
implemented in src/server/match/match_state.cpp. This interface exposes the
control surface needed by the rest of the game code to manage match state
transitions, warmup/round handling, and scoreboard calculations.
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
};

} // namespace worr::server::client
