#include "../src/server/gameplay/g_clients.cpp"
#include <gtest/gtest.h>

// Mocks
local_game_import_t gi;
GameLocals game;
game_export_t globals;
gentity_t* g_entities = nullptr;

// Menu class implementation stubs for g_local.hpp
void Menu::Next() {}
void Menu::Prev() {}
void Menu::Select(gentity_t* ent) {}
void Menu::Render(gentity_t* ent) const {}
void Menu::EnsureCurrentVisible() {}

namespace {
	// Mock malloc/free
	void* MockTagMalloc(size_t size, int tag) {
		return malloc(size);
	}

	void MockTagFree(void* ptr) {
		free(ptr);
	}

	void MockComError(const char* msg) {
		fprintf(stderr, "Com_Error: %s\n", msg);
		exit(1);
	}
}

class GClientsTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Initialize mocks
		gi.TagMalloc = MockTagMalloc;
		gi.TagFree = MockTagFree;
		gi.Com_Error = MockComError;

		// Allocate g_entities array mock
		// Assuming MAX_CLIENTS_KEX is 32, we need enough space.
		// g_entities needs to be large enough to hold maxClients + 1 entities.
		// Let's allocate 64 entities to be safe.
		g_entities = new gentity_t[64];

		// Ensure g_entities are zeroed out initially
		memset(g_entities, 0, sizeof(gentity_t) * 64);

		game.clients = nullptr;
		game.maxClients = 0;
		globals.numEntities = 0;
	}

	void TearDown() override {
		if (game.clients) {
			FreeClientArray();
		}
		delete[] g_entities;
		g_entities = nullptr;
	}
};

TEST_F(GClientsTest, AllocateAndFreeClientArray) {
	int maxClients = 4;

	// Test Allocation
	AllocateClientArray(maxClients);

	EXPECT_EQ(game.maxClients, maxClients);
	EXPECT_NE(game.clients, nullptr);
	EXPECT_EQ(globals.numEntities, maxClients + 1);

	// Verify linkage
	for (int i = 0; i < maxClients; i++) {
		EXPECT_EQ(g_entities[i + 1].client, &game.clients[i]);
	}

	// Verify FreeClientArray sets pointers to dummy client
	FreeClientArray();

	EXPECT_EQ(game.clients, nullptr);
	EXPECT_EQ(game.maxClients, 0);
	EXPECT_EQ(globals.numEntities, 1);

	// The key verification: g_entities[i+1].client should NOT be nullptr
	// It should point to the static dummy client.

	gclient_t* dummyAddress = g_entities[1].client;
	EXPECT_NE(dummyAddress, nullptr);

	for (int i = 0; i < maxClients; i++) {
		EXPECT_EQ(g_entities[i + 1].client, dummyAddress);
	}
}
