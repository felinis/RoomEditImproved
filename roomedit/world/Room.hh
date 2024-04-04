#pragma once
#include "sbfilesystem/ReadStream.hh"
#include "sbmemory/MemoryPool.hh"
#include "Vector3.hh"
#include "Light.hh"
#include "Object.hh"
#include "Mesh.hh"

struct Trigger
{
	Vector3 min;
	Vector3 max;
};

struct Room
{
	int32_t index; //index in the list of rooms
	uint32_t location;

	float scale;
	Vector3 position;

	//sound data
	int32_t sfxAmbient;
	struct SFX_ENVIRONMENT_DATA
	{
		uint16_t type;
		uint8_t delay;
		uint8_t feedback;

		constexpr SFX_ENVIRONMENT_DATA() :
			type(0),
			delay(0),
			feedback(0)
		{}
	};
	static_assert(sizeof(SFX_ENVIRONMENT_DATA) == 4);
	SFX_ENVIRONMENT_DATA sfxEnvironmentData;

	Object *objects; //linked-list of objects
	Array<Light *> lights;
	Array<Trigger> triggers;

	Array<uint32_t> viewableRooms; //indices of rooms that are visible from the current one

	Mesh mesh;

	constexpr Room() :
		index(-1),
		location(0),
		scale(1.0f),
		sfxAmbient(-1),
		objects(nullptr)
	{}

	void Load(ReadStream &rs, MemoryPool &pool, Light *worldLights, Object *worldObjects);
};
