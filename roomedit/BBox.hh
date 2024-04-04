#pragma once
#include "common/vector.inl"
#include <math.h>
//*sigh*
#undef min
#undef max

//a bounding-box
struct BBox
{
	Vector min;
	Vector max;

	BBox() = default;

	BBox(const Vector &min, const Vector &max): min(min), max(max) { }

	void Expand(const Vector &point)
	{
		min.x = fminf(min.x, point.x);
		min.y = fminf(min.y, point.y);
		min.z = fminf(min.z, point.z);

		max.x = fmaxf(max.x, point.x);
		max.y = fmaxf(max.y, point.y);
		max.z = fmaxf(max.z, point.z);
	}

	bool IsOverlapSphereBounds(const Vector &pos, float radius) const
	{
		if (pos.x > min.x && pos.x < max.x &&
			pos.y > min.y && pos.y < max.y &&
			pos.z > min.z && pos.z < max.z)
			return true;

		if (pos.x + radius < min.x) return false;
		if (pos.y + radius < min.y) return false;
		if (pos.z + radius < min.z) return false;
		if (pos.x - radius > max.x) return false;
		if (pos.y - radius > max.y) return false;
		if (pos.z - radius > max.z) return false;
		return true;
	}

	bool IsContainSphere(const Vector &pos, float radius) const
	{
		if (pos.x - radius < min.x) return false;
		if (pos.y - radius < min.y) return false;
		if (pos.z - radius < min.z) return false;
		if (pos.x + radius > max.x) return false;
		if (pos.y + radius > max.y) return false;
		if (pos.z + radius > max.z) return false;
		return true;
	}

	// Check two ortogonal bounding boxes for intersection.
	inline bool	IsIntersectBox(const BBox &b) const
	{
		// Check for intersection on X axis.
		if ((min.x > b.max.x) || (b.min.x > max.x)) return false;
		// Check for intersection on Y axis.
		if ((min.y > b.max.y) || (b.min.y > max.y)) return false;
		// Check for intersection on Z axis.
		if ((min.z > b.max.z) || (b.min.z > max.z)) return false;

		// Boxes intersect in all 3 axises.
		return true;
	}

	bool IsIntersectRay(const Vector &origin, const Vector &dir, Vector &pntContact);

	//! Check if ray intersect edge of bounding box.
	//! @param epsilonDist if distance between ray and egde is less then this epsilon then edge was intersected.
	//! @param dist Distance between ray and edge.
	//! @param intPnt intersection point.
	bool RayEdgeIntersection(const Vector &raySrc, const Vector &rayDir, float epsilonDist, float &dist, Vector &intPnt);
};
