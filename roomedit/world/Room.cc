/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "Room.hh"
#include "common.hh"

//TODO: rename to TRIGGERDATA::Load
static void ReadAdditionalPix(ReadStream &rs)
{
	rs.AdvanceBy(7);
	int unk;
	rs >> unk;
	rs.AdvanceBy((size_t)unk * 4);
	rs.AdvanceBy(16);
}

static void LoadSubObjects(ReadStream &rs, Object *&objects, Object *worldObjects, int32_t location)
{
	Object *&previous = objects;
	previous = nullptr;

	uint32_t count;
	rs >> count;
	for (uint32_t i = 0; i < count; i++)
	{
		uint32_t index;
		rs >> index;
		Object &o = worldObjects[index];
		o.index = index;
		o.location = location;
		o.next = previous;
		previous = &o;

		//load the sub-objects of that object
		LoadSubObjects(rs, o.objects, worldObjects, index | 0x10000);
	}
}

void Room::Load(ReadStream &rs, MemoryPool &pool, Light *worldLights, Object *worldObjects)
{
	rs >> scale;
	assert(scale > 0.0f && scale <= 8.0f); //sanity check
	rs >> position;
	ConvertHandedness(position);
	rs >> sfxAmbient;
	rs >> sfxEnvironmentData;

	//room's lights
	{
		uint32_t numLights;
		rs >> numLights;
		assert(numLights < 256); //sanity check
		lights = pool.CreateArray<Light *>(numLights);
		for (Light *&light : lights)
		{
			uint32_t lightIndex;
			rs >> lightIndex;
			light = &worldLights[lightIndex];
		}
	}

	//room's subobjects
	LoadSubObjects(rs, objects, worldObjects, 0xFFFFFFFF - index);

	//room's triggers
	{
		uint32_t numTriggers;
		rs >> numTriggers;
		assert(numTriggers < 128); //sanity check
		triggers = pool.CreateArray<Trigger>(numTriggers);
		for (auto &trigger : triggers)
		{
			char triggerName[MAX_DRAWABLE_NAME_LENGTH];
			LoadDrawableName(rs, triggerName);

			rs >> trigger.min;
			ConvertHandedness(trigger.min);
			rs >> trigger.max;
			ConvertHandedness(trigger.max);

			//int *n1, *n2, *n3;
			rs.AdvanceBy(12);

			bool b1;
			rs >> b1;
			if (b1)
				ReadAdditionalPix(rs);

			rs.AdvanceBy(4);
			int num1;
			rs >> num1;
			rs.AdvanceBy(num1 * 4);
		}
	}

	//room's AI networks
	{
		int numAINetworks;
		rs >> numAINetworks;
		assert(numAINetworks < 100); //sanity check
		rs.AdvanceBy(numAINetworks * 4);
	}

	//room's viewable rooms
	{
		int numViewableRooms;
		rs >> numViewableRooms;
		assert(numViewableRooms < 80); //sanity check
		viewableRooms = pool.CreateArray<uint32_t>(numViewableRooms);
		rs.Read(viewableRooms.Data(), numViewableRooms * sizeof(uint32_t)); //read directly
	}

	//room's mesh
	mesh = ReadMesh(rs, pool);
	mesh.flags |= 3;
}
