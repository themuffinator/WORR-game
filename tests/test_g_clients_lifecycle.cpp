#include "../src/server/gameplay/g_clients.hpp"

#include <cassert>
#include <type_traits>

int main() {
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

        return 0;
}
