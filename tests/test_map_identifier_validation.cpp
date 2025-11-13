#include "shared/map_validation.hpp"

#include <cassert>

int main()
{
	assert(G_IsValidMapIdentifier("q2dm1"));
	assert(!G_IsValidMapIdentifier(".."));
	assert(!G_IsValidMapIdentifier("evil/map"));
	assert(G_IsValidOverrideDirectory("maps"));
	assert(G_IsValidOverrideDirectory("custom_maps"));
	assert(!G_IsValidOverrideDirectory("../evil"));
	assert(!G_IsValidOverrideDirectory("maps/.."));
	assert(!G_IsValidOverrideDirectory("maps\\evil"));
	return 0;
}
