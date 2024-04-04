#pragma once
#include "sbMemory/MemoryPool.hh"
#include "sbfilesystem/ReadStream.hh"
#include "common/result.hh"

#include "world/common.hh"
#include "world/Vector3.hh"
#include "world/Light.hh"
#include "world/Room.hh"
#include "world/Mesh.hh"
#include "world/Emitter.hh"
#include "world/SpotEffect.hh"
#include "world/Object.hh"
#include "world/TextureInformation.hh"
#include "world/SurfaceMaterial.hh"
#include "world/SurfaceProperty.hh"
#include "world/ActorWAD.hh"

#include <assert.h>
#include <memory.h>

struct WorldHeader
{
	uint32_t version;
	int32_t levelId;
	uint32_t numPlayers;
	uint32_t flags;
	char levelName[4];
	char nextLevelName[4];
};
static_assert(sizeof(WorldHeader) == 24);

struct AINetwork
{
	
};

class GameWorld
{
	MemoryPool pool;

public:
	GameWorld() = default;

	RESULT Load(const char *filePath);

	//C++ LOVES TO CALL THE DESTRUCTOR WHEN USING std::move... copy elision, damn you!
	//THIS IS WHY INSTEAD OF USING A DESTRUCTOR, I HAVE TO WRITE A MANUAL Release() METHOD
	//AND CALL IT MANUALLY WHEN NEEDED.
	void Release()
	{
		pool.Destroy();

		//just set every attribute to 0
		memset(this, 0, sizeof(*this)); //this is bad, but C++ forces me to do this... gahhh!
	}

	//do not allow copy constructor and move constructor
	GameWorld(const GameWorld& other) = delete;
	GameWorld(GameWorld &&other) = delete;

	WorldHeader header;
	Array<int32_t> drawables;
	Array<AINetwork> aiNetworks;
	Array<Light> lights;
	Array<Room> rooms;
	Array<Room> reflectors;

	RGBAColor fogColor;

	Array<Mesh> meshes;
	Array<Emitter> emitters;
	Array<SpotEffect> spotEffects;
	Array<Object> objects;

	Array<TextureInformation> textures;
	Array<SurfaceMaterial> surfaceMaterials;
	Array<SurfaceProperty> surfaceProperties;

	ActorWAD actorWAD;

	static constexpr uint8_t NUM_SYSTEM_TEXTURES = 5;
	int32_t GetBaseTextureIndex(int32_t index_surface_property) const
	{
		assert(index_surface_property >= 0);
		index_surface_property -= NUM_SYSTEM_TEXTURES;

		int32_t textureIndex = -1;
		int32_t materialIndex;

		if ((index_surface_property < 0) || (index_surface_property >= 1024))
			materialIndex = -1;
		else if (surfaceProperties[index_surface_property].pmaterials.Count() == 0)
			materialIndex = -1;
		else
		{
			const SurfaceProperty& surfProp = surfaceProperties[index_surface_property];
			materialIndex = surfProp.pmaterials[surfProp.selected_material] - NUM_SYSTEM_TEXTURES; //TODO: possible bug
			assert(materialIndex >= 0);
		}
		if (materialIndex != -1)
			textureIndex = surfaceMaterials[materialIndex].GetBaseTextureIndex() - NUM_SYSTEM_TEXTURES;
		return textureIndex;
	}
};
