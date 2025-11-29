/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_vector3_division.cpp implementation.*/

#include <cassert>
#include <cmath>

#include "shared/q_vec3.hpp"

/*
=============
main

Validates Vector3 division behavior when divisors are zero or near-zero.
=============
*/
int main() {
	Vector3 numerator{ 1.0f, -2.0f, 3.0f };
	Vector3 nearZeroDivisor{ 0.0f, 1.0e-8f, 4.0f };

	Vector3 vectorResult = numerator / nearZeroDivisor;
	assert(std::isfinite(vectorResult.x));
	assert(std::isfinite(vectorResult.y));
	assert(vectorResult.z == 0.75f);
	const float expectedVectorX = 1.0f / Vector3::kDivisionEpsilon;
	const float expectedVectorY = -2.0f / Vector3::kDivisionEpsilon;
	assert(std::fabs(vectorResult.x - expectedVectorX) < 1.0f);
	assert(std::fabs(vectorResult.y - expectedVectorY) < 1.0f);

	Vector3 scalarResult = numerator / 0.0f;
	assert(std::isfinite(scalarResult.x));
	assert(std::isfinite(scalarResult.y));
	assert(scalarResult.z == 3.0f / Vector3::kDivisionEpsilon);
	const float expectedScalarX = 1.0f / Vector3::kDivisionEpsilon;
	const float expectedScalarY = -2.0f / Vector3::kDivisionEpsilon;
	assert(std::fabs(scalarResult.x - expectedScalarX) < 1.0f);
	assert(std::fabs(scalarResult.y - expectedScalarY) < 1.0f);

	Vector3 compoundVector = numerator;
	compoundVector /= nearZeroDivisor;
	assert(compoundVector.equals(vectorResult, 1.0f));

	Vector3 compoundScalar = numerator;
	compoundScalar /= 0.0f;
	assert(compoundScalar.equals(scalarResult, 1.0f));

	return 0;
}
