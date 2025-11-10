#include <cassert>
#include <cmath>

namespace std {
	using ::sinf;
}

#include "server/gameplay/g_harvester.hpp"

int main() {
	const Vector3 generator{ 128.0f, -64.0f, 32.0f };
	const Vector3 fallback{ -32.0f, 96.0f, 16.0f };
	const float randomX = 0.5f;
	const float randomY = -0.25f;
	const float randomZ = -0.75f;

	const Vector3 generatorDrop = Harvester_ComputeSkullOrigin(
		generator,
		fallback,
		false,
		randomX,
		randomY,
		randomZ);
	assert(std::fabs(generatorDrop.x - (generator.x + randomX * 24.0f)) < 0.001f);
	assert(std::fabs(generatorDrop.y - (generator.y + randomY * 24.0f)) < 0.001f);
	const float expectedGeneratorZ = generator.z + 16.0f + std::fabs(randomZ * 12.0f);
	assert(std::fabs(generatorDrop.z - expectedGeneratorZ) < 0.001f);

	const Vector3 fallbackDrop = Harvester_ComputeSkullOrigin(
		generator,
		fallback,
		true,
		randomX,
		randomY,
		randomZ);
	assert(std::fabs(fallbackDrop.x - fallback.x) < 0.001f);
	assert(std::fabs(fallbackDrop.y - fallback.y) < 0.001f);
	const float expectedFallbackZ = fallback.z + 16.0f + std::fabs(randomZ * 12.0f);
	assert(std::fabs(fallbackDrop.z - expectedFallbackZ) < 0.001f);

	return 0;
}
