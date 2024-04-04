/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "Light.hh"

static constexpr const char *LIGHT_TYPE_STRINGS[] =
{
	"Point",
	"Fog",
	"Dynamic",
	"Spot",
	"Fog Plane",
	"Dynamic Spot"
};

const char *Light::GetTypeString() const
{
	return LIGHT_TYPE_STRINGS[(uint32_t)type];
}
