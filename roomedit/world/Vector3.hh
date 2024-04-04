#pragma once

struct Vector3
{
	float x, y, z;
	constexpr Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
};
static_assert(sizeof(Vector3) == 12);
