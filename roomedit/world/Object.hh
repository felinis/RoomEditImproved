#pragma once
#include "Vector3.hh"
#include <stdint.h>

enum MESH_TYPE : uint32_t
{
	MT_MESH,
	MT_EMITTER,
	MT_UNUSED_SPRITE,
	MT_HIERARCHY,
	MT_TRIGGER,
	MT_SPOT_EFFECT,
	MT_END_LIST
};
class DrawableNumber
{
	uint32_t encodedNumber;

public:
	DrawableNumber(): encodedNumber(0) {}
	DrawableNumber(uint32_t id, MESH_TYPE meshType): encodedNumber(id + (meshType << 16)) {}

	inline uint32_t GetID() const
	{
		return encodedNumber & 0xFFFF;
	}

	inline MESH_TYPE GetMeshType() const
	{
		return (MESH_TYPE)(encodedNumber >> 16);
	}
};

struct Object
{
	uint32_t index; //index in the objects array
	uint32_t location; //parent room index

	Object *next;
	Object *objects;

	DrawableNumber drawableNumber;

	float radius;
	float scale;
	Vector3 position;
	Vector3 rotation;

	uint32_t type;

	Object() :
		index(0),
		location(0),
		next(nullptr),
		objects(nullptr),
		radius(1.0f),
		scale(1.0f),
		type(0)
	{}
};
