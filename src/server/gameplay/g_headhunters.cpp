#include "../g_local.hpp"
#include "g_headhunters.hpp"

#include <algorithm>
#include <limits>

namespace HeadHunters {

namespace {
	constexpr const char* kHeadModelPath = "models/objects/gibs/skull/tris.md2";
	constexpr GameTime kHeadLifetime = 30_sec;
	constexpr float kHeadHorizontalImpulse = 120.0f;
	constexpr float kHeadVerticalImpulse = 200.0f;
	constexpr GameTime kPickupCooldown = 250_ms;
	constexpr GameTime kDropCooldown = 500_ms;

	inline LevelLocals::HeadHuntersState& State() {
		return level.headHunters;
	}

	inline bool Active() {
		return Game::Is(GameType::HeadHunters);
	}

	void CleanupLooseHeads(LevelLocals::HeadHuntersState& state) {
		size_t count = 0;
		for (auto& slot : state.looseHeads) {
			if (slot && slot->inUse) {
				++count;
			}
			else {
				slot = nullptr;
			}
		}
		state.looseHeadCount = count;
	}

	void RegisterLooseHead(gentity_t* ent) {
		auto& state = State();
		for (auto& slot : state.looseHeads) {
			if (!slot || !slot->inUse) {
				slot = ent;
				CleanupLooseHeads(state);
				return;
			}
		}
		CleanupLooseHeads(state);
	}

	void UnregisterLooseHead(gentity_t* ent) {
		auto& state = State();
		for (auto& slot : state.looseHeads) {
			if (slot == ent) {
				slot = nullptr;
				break;
			}
		}
		CleanupLooseHeads(state);
	}

	void SyncClient(gclient_t* client) {
		if (!client)
			return;
		client->ps.generic1 = client->headhunter.carried;
	}

	Vector3 DropOrigin(const gentity_t* player) {
		Vector3 origin = player->s.origin;
		origin.z += player->viewHeight * 0.5f;
		return origin;
	}
} // namespace

void ClearState() {
	auto& state = State();
	for (gentity_t* ent : state.looseHeads) {
		if (ent && ent->inUse && ent->touch == HandlePickup)
			FreeEntity(ent);
	}
	state = {};
}

void ResetPlayerState(gclient_t* client) {
	if (!client)
		return;
	client->headhunter = {};
	SyncClient(client);
}

void InitLevel() {
	auto& state = State();
	state.headModelIndex = gi.modelIndex(kHeadModelPath);
	state.receptacleCount = 0;
	state.spikeCount = 0;
	CleanupLooseHeads(state);
	for (size_t i = 0; i < game.maxClients; ++i)
		ResetPlayerState(&game.clients[i]);
	if (!Active())
		return;
}

void RunFrame() {
	auto& state = State();
	CleanupLooseHeads(state);
	if (!Active())
		return;
	for (auto& receptacle : state.receptacles)
		if (receptacle.ent && !receptacle.ent->inUse)
			receptacle = {};
	for (auto& spike : state.spikeQueue)
		if (spike.ent && !spike.ent->inUse)
			spike = {};
}

gentity_t* SpawnGroundHead(const Vector3& origin, const Vector3& velocity, Team team) {
	if (!Active())
		return nullptr;
	gentity_t* head = Spawn();
	if (!head)
		return nullptr;
	head->className = "item_headhunter_head";
	head->mins = { -12.0f, -12.0f, -12.0f };
	head->maxs = { 12.0f, 12.0f, 12.0f };
	head->solid = SOLID_TRIGGER;
	head->clipMask = MASK_SOLID;
	head->moveType = MoveType::Toss;
	head->touch = HandlePickup;
	head->think = FreeEntity;
	head->nextThink = level.time + kHeadLifetime;
	head->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
	head->gravityVector = { 0.0f, 0.0f, -1.0f };
	head->s.effects |= EF_ROTATE;
	head->s.renderFX |= RF_IR_VISIBLE | RF_NO_LOD;
	switch (team) {
	case Team::Red:
		head->s.renderFX |= RF_SHELL_RED;
		break;
	case Team::Blue:
		head->s.renderFX |= RF_SHELL_BLUE;
		break;
	default:
		break;
	}
	head->fteam = team;
	head->s.origin = origin;
	head->velocity = velocity;
	head->aVelocity = { 0.0f, 0.0f, 90.0f };
	auto& state = State();
	if (!state.headModelIndex)
		state.headModelIndex = gi.modelIndex(kHeadModelPath);
	head->s.modelIndex = state.headModelIndex;
	gi.setModel(head, kHeadModelPath);
	gi.linkEntity(head);
	RegisterLooseHead(head);
	return head;
}

void DropHeads(gentity_t* player, gentity_t* instigator) {
	if (!Active())
		return;
	if (!player || !player->client)
		return;
	gclient_t* client = player->client;
	if (client->headhunter.carried == 0)
		return;
	Vector3 baseOrigin = DropOrigin(player);
	const Team team = client->sess.team;
	for (uint8_t i = 0; i < client->headhunter.carried; ++i) {
		Vector3 spawnOrigin = baseOrigin;
		spawnOrigin.x += crandom() * 12.0f;
		spawnOrigin.y += crandom() * 12.0f;
		Vector3 tossVelocity = player->velocity;
		tossVelocity.x += crandom() * kHeadHorizontalImpulse;
		tossVelocity.y += crandom() * kHeadHorizontalImpulse;
		tossVelocity.z += kHeadVerticalImpulse + frandom() * 50.0f;
		if (instigator && instigator->client)
			tossVelocity += instigator->client->vForward * 75.0f;
		SpawnGroundHead(spawnOrigin, tossVelocity, team);
	}
	client->headhunter.attachments.fill(nullptr);
	client->headhunter.carried = 0;
	client->headhunter.dropCooldown = level.time + kDropCooldown;
	SyncClient(client);
}

void HandlePickup(gentity_t* ent, gentity_t* other, const trace_t&, bool) {
	if (!Active())
		return;
	if (!ent || !other || !other->client)
		return;
	gclient_t* client = other->client;
	if (client->headhunter.dropCooldown > level.time)
		return;
	if (client->headhunter.pickupCooldown > level.time)
		return;
	if (client->headhunter.carried == std::numeric_limits<uint8_t>::max())
		return;
	++client->headhunter.carried;
	client->headhunter.pickupCooldown = level.time + kPickupCooldown;
	SyncClient(client);
	gi.sound(other, CHAN_ITEM, gi.soundIndex("items/pkup.wav"), 1.0f, ATTN_NORM, 0);
	UnregisterLooseHead(ent);
	if (ent->inUse)
		FreeEntity(ent);
}

} // namespace HeadHunters
