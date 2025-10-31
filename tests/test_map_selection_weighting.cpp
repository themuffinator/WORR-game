#include <array>
#include <cassert>
#include <cstddef>
#include <random>
#include <vector>

namespace {

std::array<int, 3> SampleSelections(std::mt19937& rng) {
        std::vector<double> weights = { 2.0, 1.0, 1.0 };
        std::discrete_distribution<size_t> distribution(weights.begin(), weights.end());

        std::array<int, 3> counts{ 0, 0, 0 };
        for (int i = 0; i < 10'000; ++i) {
                ++counts[distribution(rng)];
        }

        return counts;
}

}

int main() {
        std::mt19937 rng{ 12345u };
        auto firstSample = SampleSelections(rng);

        // Popular map (index 0) should win more often than the normal-weight entries.
        assert(firstSample[0] > firstSample[1]);
        assert(firstSample[0] > firstSample[2]);

        // Re-using the same seed must reproduce identical selection counts.
        std::mt19937 reproducibleRng{ 12345u };
        auto secondSample = SampleSelections(reproducibleRng);
        assert(firstSample == secondSample);

        return 0;
}
