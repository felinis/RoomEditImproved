#pragma once
#include "sbfilesystem/ReadStream.hh"
#include "sbmemory/MemoryPool.hh"
#include "Vector3.hh"
#include "common.hh"
#include <stdint.h>

struct Corner
{
	uint16_t index;
	uint16_t padding;
	uint32_t color;
	float textureUV[2];
	float lightmapUV[2];
};
static_assert(sizeof(Corner) == 24);

struct Face
{
	int32_t indexSurfaceProperty;
	union
	{
		int32_t lightmapIndex;		//used if this Face does not belong to an Actor
		int32_t indexActorMaterial; //used if this Face belongs to an Actor
	};

	uint32_t typePoly;

	uint32_t numVerts;
	Array<uint16_t> vertexIndices;
	Array<int32_t> mi;
	Array<int16_t> globalI;

	struct Hull
	{
		bool valid;

		int32_t charC;
		Vector3 vec0;
		Vector3 vec1;
		Vector3 normal;
		Vector3 vec3;
		Array<Vector3> vertices;

		Hull() :
			valid(false),
			charC(),
			vec0(),
			vec1(),
			normal(),
			vec3()
		{}
	};
	Hull hull;

	Vector3 normal;
	Vector3 minExtent;
	Vector3 maxExtent;
	//	Vector3 v2;
	//	Vector3 v3;

	Face() :
		indexSurfaceProperty(-1),
		lightmapIndex(-1),
		typePoly(6),
		numVerts(0),
		normal(),
		minExtent(),
		maxExtent()
	{}
};

struct Mesh
{
	char name[MAX_DRAWABLE_NAME_LENGTH];
	int32_t flags;

	uint32_t numVerts;
	Vector3 *positions;
	Vector3 *normals;
	Vector3 minExtent;
	Vector3 maxExtent;

	Array<Corner> corners;
	Array<Face> faces;

	constexpr Mesh() :
		name(),
		flags(0),
		numVerts(0),
		positions(nullptr),
		normals(nullptr),
		minExtent(),
		maxExtent()
	{}
};

Mesh ReadMesh(ReadStream &rs, MemoryPool &pool);
