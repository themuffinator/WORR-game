/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_g_clients_lifecycle.cpp implementation.*/

#include "../src/server/gameplay/g_clients.hpp"

#include <cassert>
#include <new>
#include <type_traits>

local_game_import_t gi{};
game_export_t globals{};
GameLocals game{};

namespace {
void* TestTagMalloc(size_t size, int /*tag*/) {
        return ::operator new(size);
}

void TestTagFree(void* ptr) {
        ::operator delete(ptr);
}
}

int main() {
        gi.TagMalloc = TestTagMalloc;
        gi.TagFree = TestTagFree;
        gi.Com_Error = +[](const char*) {};
        gi.frameTimeSec = 0.05f;
        globals.numEntities = 1;

        constexpr std::size_t kClientCount = 3;
        using Storage = std::aligned_storage_t<sizeof(gclient_t), alignof(gclient_t)>;
        Storage raw[kClientCount];
        auto* clients = reinterpret_cast<gclient_t*>(raw);

        ConstructClients(clients, kClientCount);
        for (std::size_t i = 0; i < kClientCount; ++i) {
                assert(!clients[i].showScores);
                assert(!clients[i].showHelp);
        }

        clients[0].showScores = true;
        DestroyClients(clients, kClientCount);

        ConstructClients(clients, kClientCount);
        for (std::size_t i = 0; i < kClientCount; ++i) {
                assert(!clients[i].showScores);
        }
        DestroyClients(clients, kClientCount);

        ClientArrayLifetime lifetime;
        lifetime.Reset(clients, kClientCount);
        clients[1].showHelp = true;
        lifetime.Reset(clients, kClientCount);
        for (std::size_t i = 0; i < kClientCount; ++i) {
                assert(!clients[i].showHelp);
        }
        lifetime.Reset(nullptr, 0);

        AllocateClientArray(4);
        assert(game.maxClients == 4);
        assert(globals.numEntities == 5);
        assert(game.clients != nullptr);
        assert(game.lagOrigins != nullptr);

        game.clients[0].showScores = true;
        ReplaceClientArray(2);
        assert(game.maxClients == 2);
        assert(globals.numEntities == 3);
        assert(!game.clients[0].showScores);

        FreeClientArray();
        assert(game.clients == nullptr);
        assert(game.lagOrigins == nullptr);
        assert(globals.numEntities == 1);

        return 0;
}
