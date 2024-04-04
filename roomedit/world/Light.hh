#pragma once
#include <stdint.h>
#include "Vector3.hh"

struct Light
{
	enum class Type : uint32_t
	{
		LIGHT,
		FOG,
		DYNAMIC_LIGHT,
		SPOTLIGHT,
		FOGPLANE,
		DYNAMIC_SPOT
	};
	Type type;
	Vector3 position;
	int32_t color[3]; //rgb
	float intensity;
	float falloff;
	float falloff_cos;
	float hotspot;
	float hotspot_cos;
	Vector3 direction;
	float ambient;

	const char *GetTypeString() const;
};
static_assert(sizeof(Light) == 64);
