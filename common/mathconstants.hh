#pragma once

namespace MathConstants
{
	constexpr float PI = 3.14159265f;

	constexpr float EPSILON = 1024.0f * 1024.0f;
	constexpr float FLOAT_MAX = EPSILON;
	constexpr Vector MIN_WORLD_BOUND = Vector(-EPSILON, -EPSILON, -EPSILON);
	constexpr Vector MAX_WORLD_BOUND = Vector(EPSILON, EPSILON, EPSILON);
}
