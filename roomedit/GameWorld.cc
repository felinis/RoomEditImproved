/*
*	Sabre World Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "GameWorld.h"
#include <utility> //std::move
#include <optional>

static Array<int> ReadDrawables(ReadStream& rs, MemoryPool& pool)
{
	//read the count
	uint32_t numDrawables;
	rs >> numDrawables;
	assert(numDrawables <= 356); //sanity check

	Array<int> array = std::move(pool.CreateArray<int>(numDrawables));
	rs.Read(array.Data(), numDrawables * sizeof(int)); //read directly all the drawables

	return std::move(array);
}

static Array<AINetwork> ReadAINetworks(ReadStream& rs, MemoryPool& pool)
{
	//read the count
	uint32_t numAINetworks;
	rs >> numAINetworks;
	assert(numAINetworks == 0); //TEMPORARY!!!

	Array<AINetwork> array = std::move(pool.CreateArray<AINetwork>(numAINetworks));

	return std::move(array);
}

static Array<Light> ReadLights(ReadStream &rs, MemoryPool& pool)
{
	//read the count
	uint32_t numLights;
	rs >> numLights;
	assert(numLights <= 256); //sanity check

	Array<Light> array = std::move(pool.CreateArray<Light>(numLights));
	rs.Read(array.Data(), numLights * sizeof(Light)); //read directly all the lights

	return std::move(array);
}

static void ReadRecursiveSec(ReadStream& rs)
{
	char bRead;
	rs >> bRead;
	if (bRead)
	{
		rs.AdvanceBy(9);

		short unk10;
		rs >> unk10;
		if (unk10)
			rs.AdvanceBy((size_t)unk10 * 2);

		/* dwa razy! */
		ReadRecursiveSec(rs);
		ReadRecursiveSec(rs);
	}
}

static Face LoadFace(ReadStream &rs, MemoryPool &pool)
{
	Face face;

	rs >> face.indexSurfaceProperty;
	rs >> face.lightmapIndex;
	rs >> face.typePoly;

	rs >> face.numVerts;
	face.vertexIndices = std::move(pool.CreateArray<unsigned short>(face.numVerts));
	rs.Read(face.vertexIndices.Data(), face.numVerts * sizeof(unsigned short)); //read directly

	uint16_t numI;
	rs >> numI;
	face.mi = std::move(pool.CreateArray<int>(numI));
	rs.Read(face.mi.Data(), numI * sizeof(int)); //read directly

	//load corner indices
	uint32_t numGlobalI;
	rs >> numGlobalI;
	face.globalI = std::move(pool.CreateArray<short>(numGlobalI));
	rs.Read(face.globalI.Data(), numGlobalI * sizeof(short)); //read directly

	//load hull
	uint32_t num_unk4;
	rs >> num_unk4;
	if (num_unk4 == 0)
	{
		rs >> face.hull.charC;
		rs >> face.hull.vec0;
		rs >> face.hull.vec1;
		rs >> face.hull.normal;
		rs >> face.hull.vec3;

		uint32_t numVertices;
		rs >> numVertices;
		face.hull.vertices = std::move(pool.CreateArray<Vector3>(numVertices));
		rs.Read(face.hull.vertices.Data(), numVertices * sizeof(Vector3)); //read directly

		//check validity of that hull
		face.hull.valid = face.hull.charC < 0 || face.indexSurfaceProperty != 3;
	}

	rs >> face.normal;
	rs >> face.minExtent;
	rs >> face.maxExtent;

	bool unk5;
	rs >> unk5;
	if (unk5)
	{
		short unk6, unk7;
		rs >> unk6;
		rs >> unk7;
		rs.AdvanceBy((size_t)unk6 * 2);
		rs.AdvanceBy((size_t)unk7 * 2);

		short unk8;
		rs >> unk8;
		rs.AdvanceBy((size_t)unk8 * 4);
	}

	return face;
}

static void LoadDrawableName(ReadStream &rs)
{
	uint32_t nameLength;
	rs >> nameLength;
	assert(nameLength < 32);
	char name[32];
	rs.Read(name, nameLength);
//	OutputDebugString(name);
//	OutputDebugString("\n");
}

static Mesh ReadMesh(ReadStream& rs, MemoryPool& pool)
{
	Mesh mesh;

	LoadDrawableName(rs);

	rs >> mesh.flags;

	rs >> mesh.numVerts;
	assert(mesh.numVerts <= 8000); //sanity check
	uint32_t size = mesh.numVerts * sizeof(Vector3);
	mesh.positions = pool.Allocate<Vector3>(mesh.numVerts);
	rs.Read(mesh.positions, size);
	mesh.normals = pool.Allocate<Vector3>(mesh.numVerts);
	rs.Read(mesh.normals, size);

	rs.AdvanceBy(4);

	rs >> mesh.minExtent;
	rs >> mesh.maxExtent;

	uint32_t numCorners;
	rs >> numCorners;
	assert(numCorners <= 4000); //sanity check
	if (numCorners == 0)
		rs.AdvanceBy(4);
	else
	{
		mesh.corners = std::move(pool.CreateArray<Corner>(numCorners));
		rs.Read(mesh.corners.Data(), numCorners * sizeof(Corner)); //read directly

		uint32_t numFaces;
		rs >> numFaces;
		assert(numFaces <= 2000); //sanity check
		mesh.faces = std::move(pool.CreateArray<Face>(numFaces));
		for (Face& face : mesh.faces)
			face = std::move(LoadFace(rs, pool));
	}

	int numCornersTextureAxis;
	rs >> numCornersTextureAxis;
	struct TEXTURE_AXIS
	{
		Vector3 axis_u;
		Vector3 axis_v;
		Vector3 axis_w;
	};
	rs.AdvanceBy(numCornersTextureAxis * sizeof(TEXTURE_AXIS));

	bool hasLocator;
	rs >> hasLocator;
	if (hasLocator)
		rs.AdvanceBy(24);

	//HIT_DATA
	short unk5;
	rs >> unk5;
	if (unk5)
		ReadRecursiveSec(rs);

	return mesh;
}

static void ReadSubObjects(ReadStream& rs, MemoryPool& pool)
{
	uint32_t num_alphabet;
	rs >> num_alphabet;
	for (uint32_t i = 0; i < num_alphabet; i++)
	{
		rs.AdvanceBy(4);
		ReadSubObjects(rs, pool);
	}
}

static void ReadAdditionalPix(ReadStream &rs)
{
	rs.AdvanceBy(7);
	int unk;
	rs >> unk;
	rs.AdvanceBy((size_t)unk * 4);
	rs.AdvanceBy(16);
}

static Array<Room> ReadRooms(ReadStream& rs, MemoryPool& pool, Light* worldLights)
{
	//read the room count
	uint32_t numRooms;
	rs.Read(&numRooms, 4);
	assert(numRooms <= 256); //sanity check

	Array<Room> array = std::move(pool.CreateArray<Room>(numRooms));
	for (Room &room : array)
	{
		rs >> room.scale;
		rs >> room.position;
		rs >> room.sfxAmbient;
		rs >> room.sfxEnvironmentData;

		//room's lights
		{
			uint32_t numLights;
			rs >> numLights;
			assert(numLights < 256); //sanity check
			room.lights = std::move(pool.CreateArray<Light *>(numLights));
			for (Light*& light : room.lights)
			{
				uint32_t lightIndex;
				rs >> lightIndex;
				light = &worldLights[lightIndex];
			}
		}

		//room's subobjects
		ReadSubObjects(rs, pool);

		//room's triggers
		{
			uint32_t numTriggers;
			rs >> numTriggers;
			assert(numTriggers < 128); //sanity check
			room.triggers = std::move(pool.CreateArray<Trigger>(numTriggers));
			for (auto& trigger : room.triggers)
			{
				LoadDrawableName(rs);

				rs >> trigger.min;
				rs >> trigger.max;

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

		//room's ai networks
		{
			int numAINetworks;
			rs >> numAINetworks;
			rs.AdvanceBy(numAINetworks * 4);
		}

		//room's viewable rooms
		{
			int numViewableRooms;
			rs >> numViewableRooms;
			rs.AdvanceBy(numViewableRooms * 4);
		}

		//room's mesh
		room.mesh = std::move(ReadMesh(rs, pool));
		room.mesh.flags |= 3;
	}

	return std::move(array);
}

static Array<Room> MakeReflectors(ReadStream &rs, MemoryPool &pool)
{
	//read the room count
	uint32_t numReflectors;
	rs >> numReflectors;
	assert(numReflectors <= 256); //sanity check

	Array<Room> array = std::move(pool.CreateArray<Room>(numReflectors));
	for (auto& room : array)
	{
		//TODO!!!
	}

	return std::move(array);
}

static Array<Mesh> ReadMeshes(ReadStream &rs, MemoryPool &pool)
{
	uint32_t numMeshes;
	rs >> numMeshes;
	assert(numMeshes <= 256); //sanity check

	Array<Mesh> array = std::move(pool.CreateArray<Mesh>(numMeshes));
	for (auto &mesh : array)
		mesh = ReadMesh(rs, pool);

	return std::move(array);
}

static std::optional<uint32_t> GetRoomTexturesStartAddress(char *levelName)
{
	int levelID = levelName[0] | (levelName[1] << 8) | (levelName[2] << 16) | (levelName[3] << 24);
	switch (levelID)
	{
	case 'ltit': /* titlebackdrop */
		return 0x61538;
	case '10tc': /* cutscene01 */
		return 0x43F56A;
	case 'zalP': /* l1_plaza */
		return 0xA2A555;
	case 'tcaF': /* l2_factory */
		return 0x8F49CD;
	case 'snoC': /* l3_construction */
		return 0xA664C7;
	case 'pohS': /* l4_shoppingmall */
		return 0xC072F9;
	case 'gnaG': /* l5_gang */
		return 0xA43219;
	case '60tc': /* cutscene06 */
		return 0x567AA3;
	case 'psoH': /* l6_hospital */
		return 0xE48F4B;
	case '70tc': /* cutscene07 */
		return 0x403675;
	case 'TykS': /* l7_skytran */
		return 0xBAC844;
	case '80tc': /* cutscene08 */
		return 0x4116C4;
	case 'ooZ': /* l8_zoo */
		return 0xBA79DD;
	case '90tc': /* cutscene09 */
		return 0x3B7AD2;
	case 'vacS': /* l9_scavenge */
		return 0xD875EB;
	case 'nnuT': /* l10_tunnels */
		return 0xD778A0;
	case 'nedE': /* l11_eden */
		return 0xD258FD;
	case 'dntc': /* cutscene_end */
		return 0x400DE2;
	case '10RR': /* roverracing_pc */
		return 0x6875D1;
	default:
		return std::nullopt;
	}
}

bool GameWorld::Load(const char* filePath)
{
	pool.Create(1024 * 1024 * 20);

	ReadStream rs;
	if (!rs.Open(filePath))
		return false;

	//read the world header
	rs >> worldHeader;
	
	//check version
	if (worldHeader.version != 72)
		return false;

	//skip one byte
	rs.AdvanceBy(1);

	drawables = std::move(ReadDrawables(rs, pool));
	aiNetworks = std::move(ReadAINetworks(rs, pool));
	lights = std::move(ReadLights(rs, pool));

	uint32_t numObjects;
	rs >> numObjects;
	//TODO: create empty array of objects
//	for (uint32_t i = 0; i < numObjects; i++)
//		objects[i].index = i;

	rooms = std::move(ReadRooms(rs, pool, lights.Data()));
	reflectors = std::move(MakeReflectors(rs, pool));

	rs >> fogColor;

//	meshes = std::move(ReadMeshes(rs, pool));

	//TODO: load the objects

	//TEMP: go to textures directly
	uint32_t address = GetRoomTexturesStartAddress(worldHeader.levelName).value();
	rs.AdvanceTo(address);

	//load texture wad
	uint32_t numSystemTextures;
	rs >> numSystemTextures;
	assert(numSystemTextures == 5);
	uint32_t numTextures;
	rs >> numTextures;
	assert(numTextures <= 2048);

	textures = std::move(pool.CreateArray<TextureInformation>(numTextures));

	//load the system textures
//	for (uint32_t i = 0; i < numSystemTextures; i++)
//	{
//		auto& ti = textures[i];
//		surface_properties[0].pmaterials = i;
//		surface_materials[0x14] = 1;
//		surface_materials = i;
//	}

	//load the usual textures
	for (uint32_t i = 0; i < numTextures; i++)
	{
		auto& ti = textures[i];
		ti.Load(rs, pool);
	}

	//load surface materials
	{
		uint32_t nsurface_materials;
		rs >> nsurface_materials;
		surfaceMaterials = std::move(pool.CreateArray<SurfaceMaterial>(nsurface_materials));
		for (auto& sm : surfaceMaterials)
			sm.Load(rs);
	}

	//load surface properties
	{
		uint32_t nsurface_properties;
		rs >> nsurface_properties;
		surfaceProperties = std::move(pool.CreateArray<SurfaceProperty>(nsurface_properties));
		for (auto& sp : surfaceProperties)
			sp.Load(rs, pool);
	}

	rs.Close();
	return true;
}

void GameWorld::Unload()
{
	pool.Destroy();
}
