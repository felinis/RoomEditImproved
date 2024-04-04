/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "BBox.hh"
#include "common/mathconstants.hh"
#include <float.h>

typedef unsigned int udword;

//integer representation of a floating-point value
#define IR(x)	((udword&)x)

#define RAYAABB_EPSILON 0.00001f
//fast bounding-box check
//this algorithm comes from one of Mr. Pierre Tardiman's papers
bool BBox::IsIntersectRay(const Vector &vOrigin, const Vector &vDir, Vector &pntContact)
{
	bool Inside = true;
	float MinB[3] = { min.x,min.y,min.z };
	float MaxB[3] = { max.x,max.y,max.z };
	float MaxT[3];
	MaxT[0] = MaxT[1] = MaxT[2] = -1.0f;
	float coord[3] = { 0,0,0 };

	float origin[3] = { vOrigin.x,vOrigin.y,vOrigin.z };
	float dir[3] = { vDir.x,vDir.y,vDir.z };

	//find candidate planes.
	for (udword i = 0; i < 3; i++)
	{
		if (origin[i] < MinB[i])
		{
			coord[i] = MinB[i];
			Inside = false;

			//calculate T distances to candidate planes
			if (IR(dir[i]))	MaxT[i] = (MinB[i] - origin[i]) / dir[i];
		}
		else if (origin[i] > MaxB[i])
		{
			coord[i] = MaxB[i];
			Inside = false;

			//calculate T distances to candidate planes
			if (IR(dir[i]))	MaxT[i] = (MaxB[i] - origin[i]) / dir[i];
		}
	}

	//ray origin inside bounding box
	if (Inside)
	{
		//this need to be treated as intersection
		pntContact = Vector(origin[0], origin[1], origin[2]);
		return true;
	}

	//get largest of the maxT's for final choice of intersection
	udword WhichPlane = 0;
	if (MaxT[1] > MaxT[WhichPlane])	WhichPlane = 1;
	if (MaxT[2] > MaxT[WhichPlane])	WhichPlane = 2;

	//check final candidate actually inside box
	if (IR(MaxT[WhichPlane]) & 0x80000000) return false;

	for (udword i = 0; i < 3; i++)
	{
		if (i != WhichPlane)
		{
			coord[i] = origin[i] + MaxT[WhichPlane] * dir[i];
#ifdef RAYAABB_EPSILON
			if (coord[i] < MinB[i] - RAYAABB_EPSILON || coord[i] > MaxB[i] + RAYAABB_EPSILON)	return false;
#else
			if (coord[i] < MinB[i] || coord[i] > MaxB[i])	return false;
#endif
		}
	}

	pntContact = Vector(coord[0], coord[1], coord[2]);
	return true; // ray hits box
}

inline float PointToLineDistance(const Vector &p1, const Vector &p2, const Vector &p3)
{
	Vector d = p2 - p1;
	float u = d.Dot(p3 - p1) / d.LengthSquared();
	if (u < 0)
		return (p3 - p1).Length();
	else if (u > 1)
		return (p3 - p2).Length();
	else
	{
		Vector p = p1 + d * u;
		return (p3 - p).Length();
	}
}

/** Calculate distance between point and line.
	@param p1 Source line point.
	@param p2 Target line point.
	@param p3 Point to find intersecion with.
	@param intersectPoint Intersection point on the line.
	@return Distance between point and line.
*/
inline float PointToLineDistance(const Vector &p1, const Vector &p2, const Vector &p3, Vector &intersectPoint)
{
	Vector d = p2 - p1;
	float u = d.Dot(p3 - p1) / d.LengthSquared();
	if (u < 0)
	{
		intersectPoint = p1;
		return (p3 - p1).Length();
	}
	else if (u > 1)
	{
		intersectPoint = p2;
		return (p3 - p2).Length();
	}
	else
	{
		Vector p = p1 + d * u;
		intersectPoint = p;
		return (p3 - p).Length();
	}
}

/**
   Calculate the line segment PaPb that is the shortest route between
   two lines P1P2 and P3P4. Calculate also the values of mua and mub where
	  Pa = P1 + mua (P2 - P1)
	  Pb = P3 + mub (P4 - P3)

	@param p1 Source point of first line.
	@param p2 Target point of first line.
	@param p3 Source point of second line.
	@param p4 Target point of second line.
	@return FALSE if no solution exists.
*/
static constexpr float LINE_EPS = 0.00001f;
inline bool LineLineIntersect(const Vector &p1, const Vector &p2, const Vector &p3, const Vector &p4,
	Vector &pa, Vector &pb, float &mua, float &mub)
{
	Vector p13, p43, p21;
	float d1343, d4321, d1321, d4343, d2121;
	float numer, denom;

	p13.x = p1.x - p3.x;
	p13.y = p1.y - p3.y;
	p13.z = p1.z - p3.z;
	p43.x = p4.x - p3.x;
	p43.y = p4.y - p3.y;
	p43.z = p4.z - p3.z;
	if (fabsf(p43.x) < LINE_EPS && fabsf(p43.y) < LINE_EPS && fabsf(p43.z) < LINE_EPS)
		return false;
	p21.x = p2.x - p1.x;
	p21.y = p2.y - p1.y;
	p21.z = p2.z - p1.z;
	if (fabsf(p21.x) < LINE_EPS && fabsf(p21.y) < LINE_EPS && fabsf(p21.z) < LINE_EPS)
		return false;

	d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
	d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
	d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
	d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
	d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

	denom = d2121 * d4343 - d4321 * d4321;
	if (fabsf(denom) < LINE_EPS)
		return false;
	numer = d1343 * d4321 - d1321 * d4343;

	mua = numer / denom;
	mub = (d1343 + d4321 * (mua)) / d4343;

	pa.x = p1.x + mua * p21.x;
	pa.y = p1.y + mua * p21.y;
	pa.z = p1.z + mua * p21.z;
	pb.x = p3.x + mub * p43.x;
	pb.y = p3.y + mub * p43.y;
	pb.z = p3.z + mub * p43.z;

	return true;
}

/*!
		Calculates shortest distance between ray and a arbitary line segment.
		@param raySrc Source point of ray.
		@param rayTrg Target point of ray.
		@param p1 First point of line segment.
		@param p2 Second point of line segment.
		@param intersectPoint This parameter returns nearest point on line segment to ray.
		@return distance fro ray to line segment.
*/
inline static float RayToLineDistance(const Vector &raySrc, const Vector &rayTrg, const Vector &p1, const Vector &p2, Vector &nearestPoint)
{
	Vector intPnt;
	Vector rayLineP1 = raySrc;
	Vector rayLineP2 = rayTrg;
	Vector pa, pb;
	float ua, ub;

	if (!LineLineIntersect(p1, p2, rayLineP1, rayLineP2, pa, pb, ua, ub))
		return MathConstants::FLOAT_MAX;

	float d = 0;
	if (ua < 0)
		d = PointToLineDistance(rayLineP1, rayLineP2, p1, intPnt);
	else if (ua > 1)
		d = PointToLineDistance(rayLineP1, rayLineP2, p2, intPnt);
	else
	{
		intPnt = rayLineP1 + Vector(rayLineP2 - rayLineP1) * ub;
		d = (pb - pa).Length();
	}
	nearestPoint = intPnt;
	return d;
}

bool BBox::RayEdgeIntersection(const Vector &raySrc, const Vector &rayDir, float epsilonDist, float &dist, Vector &intPnt)
{
	//check 6 group lines
	Vector rayTrg = raySrc + rayDir * MathConstants::EPSILON;
	Vector pnt[12];

	float d[12];

	//near
	d[0] = RayToLineDistance(raySrc, rayTrg, Vector(min.x, min.y, max.z), Vector(max.x, min.y, max.z), pnt[0]);
	d[1] = RayToLineDistance(raySrc, rayTrg, Vector(min.x, max.y, max.z), Vector(max.x, max.y, max.z), pnt[1]);
	d[2] = RayToLineDistance(raySrc, rayTrg, Vector(min.x, min.y, max.z), Vector(min.x, max.y, max.z), pnt[2]);
	d[3] = RayToLineDistance(raySrc, rayTrg, Vector(max.x, min.y, max.z), Vector(max.x, max.y, max.z), pnt[3]);

	//far
	d[4] = RayToLineDistance(raySrc, rayTrg, Vector(min.x, min.y, min.z), Vector(max.x, min.y, min.z), pnt[4]);
	d[5] = RayToLineDistance(raySrc, rayTrg, Vector(min.x, max.y, min.z), Vector(max.x, max.y, min.z), pnt[5]);
	d[6] = RayToLineDistance(raySrc, rayTrg, Vector(min.x, min.y, min.z), Vector(min.x, max.y, min.z), pnt[6]);
	d[7] = RayToLineDistance(raySrc, rayTrg, Vector(max.x, min.y, min.z), Vector(max.x, max.y, min.z), pnt[7]);

	//sides
	d[8] = RayToLineDistance(raySrc, rayTrg, Vector(min.x, min.y, min.z), Vector(min.x, min.y, max.z), pnt[8]);
	d[9] = RayToLineDistance(raySrc, rayTrg, Vector(max.x, min.y, min.z), Vector(max.x, min.y, max.z), pnt[9]);
	d[10] = RayToLineDistance(raySrc, rayTrg, Vector(min.x, max.y, min.z), Vector(min.x, max.y, max.z), pnt[10]);
	d[11] = RayToLineDistance(raySrc, rayTrg, Vector(max.x, max.y, min.z), Vector(max.x, max.y, max.z), pnt[11]);

	dist = MathConstants::FLOAT_MAX;
	for (int i = 0; i < 12; i++)
	{
		if (d[i] < dist)
		{
			dist = d[i];
			intPnt = pnt[i];
		}
	}
	if (dist < epsilonDist)
	{
		return true;
	}
	return false;
}
