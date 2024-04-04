/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "GameWorld.hh"
#include "common/result.hh"
#include "world/ObjectProperties.hh"
#include <utility> //std::move

static Array<Room> ReadRooms(ReadStream& rs, MemoryPool& pool, Light* worldLights, Object *worldObjects)
{
	//read the room count
	uint32_t numRooms;
	rs.Read(&numRooms, 4);
	assert(numRooms <= 256); //sanity check

	Array<Room> array = std::move(pool.CreateArray<Room>(numRooms));
	for (Room &room : array)
		room.Load(rs, pool, worldLights, worldObjects);

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

static bool ReadMeshes(Array<Mesh> &meshes, ReadStream &rs, MemoryPool &pool)
{
	uint32_t numMeshes;
	rs >> numMeshes;
	if (numMeshes > 256) //sanity check
		return false;

	meshes = std::move(pool.CreateArray<Mesh>(numMeshes));
	for (auto &mesh : meshes)
		mesh = ReadMesh(rs, pool);

	return true;
}

static bool ReadEmitters(Array<Emitter> &emitters, ReadStream &rs, MemoryPool &pool)
{
	uint32_t numEmitters;
	rs >> numEmitters;
	if (numEmitters > 256) //sanity check
		return false;

	emitters = std::move(pool.CreateArray<Emitter>(numEmitters));
	for (auto &e : emitters)
		e.Load(rs, pool);

	return true;
}

static bool ReadSpotEffects(Array<SpotEffect> &spotEffects, ReadStream &rs, MemoryPool &pool)
{
	uint32_t numSpotEffects;
	rs >> numSpotEffects;
	if (numSpotEffects > 300) //sanity check
		return false;

	spotEffects = std::move(pool.CreateArray<SpotEffect>(numSpotEffects));
	for (auto &e : spotEffects)
		e.Load(rs, pool);

	return true;
}

struct WP_ANIM
{
	struct WP_POS
	{
		float time;
		float time_mod;
		int wp;
		int last_wp;
		int link;
	};
	WP_POS pos;

	uint32_t system_state;
	struct WP
	{
		enum WPS_STATE
		{
			WPS_SYSTEM = 0,
			WPS_TRIGGER = 1,
			WPS_TRIGGEROFF = 2,
			WPS_OFF = 3,
			WPS_ON = 4,
			WPS_CLOSED = 5,
			WPS_OPEN = 6,
			WPS_LEVEL1 = 7,
			WPS_LEVEL2 = 8,
			WPS_LEVEL3 = 9,
			WPS_LEVEL4 = 10,
			WPS_WAITING = 11,
			WPS_READY = 12,
			WPS_MOVING = 13,
			WPS_BROKEN = 14,
			WPS_NUMBER = 15
		};
		WPS_STATE state;
		struct WP_LINK
		{
			WPS_STATE state;
			bool check_conditions;
			bool hold;
			bool gravity;

			struct WP_CONDITION
			{
				uint32_t state_index;
			};
			Array<WP_CONDITION> conditions;

			int next;
			float nframes;
			float ease_to;
			float ease_from;

			void Load(ReadStream &rs, MemoryPool &pool)
			{
				rs >> state;
				rs >> check_conditions;
				rs >> hold;
				rs >> gravity;

				//load conditions
				uint32_t nconditions;
				rs >> nconditions;
				conditions = pool.CreateArray<WP_CONDITION>(nconditions);
				rs.Read(conditions.Data(), nconditions * sizeof(WP_CONDITION));

				rs >> next;
				rs >> nframes;
				rs >> ease_to;
				rs >> ease_from;
			}
		};
		Array<WP_LINK> links;

		void Load(ReadStream &rs, MemoryPool &pool)
		{
			uint32_t s;
			rs >> s;

			//load links
			uint32_t nlinks;
			rs >> nlinks;
			links = pool.CreateArray<WP_LINK>(nlinks);
			for (auto &link : links)
				link.Load(rs, pool);
		}
	};
	Array<WP> waypoints;

//	WP_ANIM_DATA *data;

	float accdec;
	bool looping;
	bool ai_network;
	bool no_deviation;

	void Load(ReadStream &rs, MemoryPool &pool)
	{
		uint32_t nwaypoints;
		rs >> nwaypoints;
		waypoints = std::move(pool.CreateArray<WP>(nwaypoints));
		for (auto &wp : waypoints)
		{
			wp.Load(rs, pool);
		}

		rs >> pos.wp;

		rs >> accdec;
		rs >> ai_network;
		rs >> no_deviation;

		uint32_t type;
		rs >> type;
		while (type)
		{
			switch (type)
			{
			case 1: //WP_OBJECT_ANIM::Load
				for (uint32_t i = 0; i < nwaypoints; i++)
				{
					bool invisible;
					rs >> invisible;
					Vector3 position, rotation;
					rs >> position;
					rs >> rotation;
					uint32_t location;
					rs >> location;
					float spin;
					rs >> spin;
				}
				break;
			case 2: //WP_TEXTURE_ANIM::Load
			{
				uint32_t propertyIndex;
				rs >> propertyIndex;
				for (uint32_t i = 0; i < nwaypoints; i++)
				{
					uint32_t material;
					rs >> material;
					float u_offset;
					rs >> u_offset;
					float v_offset;
					rs >> v_offset;
					float roll;
					rs >> roll;
					float r;
					rs >> r;
					float g;
					rs >> g;
					float b;
					rs >> b;
				}
				break;
			}
			case 3: //WP_SOUND_ANIM::Load
				for (uint32_t i = 0; i < nwaypoints; i++)
				{
					uint32_t arrivalSoundID, leavingSoundID;
					rs >> arrivalSoundID;
					rs >> leavingSoundID;
				}
				break;
			case 4: //WP_CHARACTER_ANIM::Load
				for (uint32_t i = 0; i < nwaypoints; i++)
				{
					//TODO: add the actual enums
					uint32_t type, speed, state;
					rs >> type;
					rs >> speed;
					rs >> state;
					uint16_t stateData;
					rs >> stateData;
					bool joinable;
					rs >> joinable;
				}
				break;
			default:
				assert(0);
				break;
			}

			rs >> type;
		};
	}
};

static bool ReadObjects(Array<Object> &objects, ReadStream &rs, MemoryPool &pool)
{
	for (auto &o : objects)
	{
		MESH_TYPE meshType;
		rs >> meshType;
		assert(meshType < 1000); //sanity check
		uint32_t actorID;
		rs >> actorID;
		assert(actorID < 1000); //sanity check
		o.drawableNumber = DrawableNumber(actorID, meshType);
		rs >> o.radius;
		assert(o.radius >= 0.0f);
		rs >> o.scale;
		assert(o.scale > 0.0f && o.scale <= 8.0f);
		rs >> o.position;
		ConvertHandedness(o.position);
		rs >> o.rotation;
		ConvertRotation(o.rotation);
		rs >> o.type;

		rs.AdvanceBy(4);

		uint32_t type;
		rs >> type;
		while (type)
		{
			switch (type)
			{
			case 2: //OBJECT ANIM
			{
				WP_ANIM wpAnim;
				wpAnim.Load(rs, pool);
				break;
			}
			case 3: //TRIGGER
			{
				uint32_t s1, s2;
				rs >> s1;
				rs >> s2;
				rs.AdvanceBy(4 * s2);
				break;
			}
			case 5:
			{
//				o.immovable &= 0x7f;
				break;
			}
			case 8:
			{
				OBJECT_PROPERTIES properties;
				properties.Load(rs, pool);
				break;
			}
			case 14: //POSITION_SOURCE_TARGET
			{
				Vector3 sourcePosition, targetPosition;
				rs >> sourcePosition;
				rs >> targetPosition;
				break;
			}
			case 15: //EMITTER deflection plane?
			{
				Vector3 v;
				rs >> v;
				uint32_t a;
				rs >> a;
				break;
			}
			//case 18 exists only in final version of the game
			case 18: //TEXTURE_SCALES::Load
			{
				uint32_t numTextureScales;
				rs >> numTextureScales;
				rs.AdvanceBy(numTextureScales * 36);
//				Mesh m;
//				m = ReadMesh(rs, pool);
				break;
			}
			default:
				break;
			}

			rs >> type;
			assert(type < 20);
		};
	}

	return true;
}

static uint32_t GetRoomTexturesStartAddress(const char *levelName)
{
	int levelID = levelName[0] | (levelName[1] << 8) | (levelName[2] << 16) | (levelName[3] << 24);
	switch (levelID)
	{
	//SINGLEPLAYER LEVELS
	case 'ltit': /* titlebackdrop */
		return 0x61552; //NOTE: the one from Eden Demo version is 0x61538
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

	//MULTIPLAYER LEVELS
	case '10RR': /* roverracing_pc */
		return 0x6875D1;
	case '10fc': /* CaptureFlag_pc */
		return 0x6CAED3;
	case '20fc': /* CaptureFlag_02_pc */
		return 0x62B603;
	case '10MD':
		return 0x3A06F4;
	case '20MD':
		return 0x602F75;
	case '30md':
		return 0x37F442;
	case '40md':
		return 0x3A3540;
	case '50md':
		return 0x389D65;
	case '60MD':
		return 0x62B6FD;
	case '70MD':
		return 0x684C8E;
	case '80md':
		return 0x5F36B1;

	default:
		return 0;
	}
}

RESULT GameWorld::Load(const char *filePath)
{
	MemoryPool pool;
	pool.Create(1024 * 1024 * 20);

	ReadStream rs;
	if (!rs.Open(filePath))
		return RESULT::CODE::FILE_FAILED_TO_OPEN;

	//read the world header
	rs >> header;
	
	//check version
	if (header.version != 72)
	{
		if (header.version == 'PMOC')
			return RESULT::CODE::WORLD_COMPRESSED;
		else
			return RESULT::CODE::WORLD_VERSION_INCORRECT;
	}

	//skip one byte
	rs.AdvanceBy(1);

	//read drawables
	{
		//read the count
		uint32_t numDrawables;
		rs >> numDrawables;
		assert(numDrawables <= 356); //sanity check

		drawables = std::move(pool.CreateArray<int>(numDrawables));
		rs.Read(drawables.Data(), numDrawables * sizeof(int)); //read directly all the drawables
	}

	//read AI networks
	{
		//read the count
		uint32_t numAINetworks;
		rs >> numAINetworks;

		aiNetworks = std::move(pool.CreateArray<AINetwork>(numAINetworks));
		for (auto &an : aiNetworks)
		{
			WP_ANIM wpAnim;
			wpAnim.Load(rs, pool);
		}
	}

	//read lights
	{
		//read the count
		uint32_t numLights;
		rs >> numLights;
		assert(numLights <= 300); //sanity check

		lights = std::move(pool.CreateArray<Light>(numLights));
		rs.Read(lights.Data(), numLights * sizeof(Light)); //read directly all the lights
		for (auto &light : lights)
		{
			ConvertHandedness(light.position);
			ConvertHandedness(light.direction);
		}
	}

	//pre-initialise objects
	{
		uint32_t numObjects;
		rs >> numObjects;
		objects = std::move(pool.CreateArray<Object>(numObjects));

		//just initialise their index
		for (uint32_t i = 0; i < numObjects; i++)
			objects[i].index = i;
	}

	//read rooms
	{
		rooms = std::move(ReadRooms(rs, pool, lights.Data(), objects.Data()));
		reflectors = std::move(MakeReflectors(rs, pool));
	}

	//read the fog color
	RGBAColor fogColor;
	rs >> fogColor;

	//read meshes
	{
		if (!ReadMeshes(meshes, rs, pool))
			return RESULT::CODE::WORLD_MESHES_FAILED_TO_LOAD;
	}

	//read emitters
	{
		if (!ReadEmitters(emitters, rs, pool))
			return RESULT::CODE::WORLD_EMITTERS_FAILED_TO_LOAD;
	}

	//read spoteffects
	{
		if (!ReadSpotEffects(spotEffects, rs, pool))
			return RESULT::CODE::WORLD_SPOT_EFFECTS_FAILED_TO_LOAD;
	}

	//read objects
	{
		if (!ReadObjects(objects, rs, pool))
			return RESULT::CODE::WORLD_OBJECTS_FAILED_TO_LOAD;
	}

	//TEMP: go to textures directly
//	if (!actorWAD.Load(rs, pool))
//		return RESULT::CODE::WORLD_OBJECTS_FAILED_TO_LOAD;

	//read textures
	{
		uint32_t address = GetRoomTexturesStartAddress(header.levelName);
		if (address == 0)
			return RESULT::CODE::WORLD_TEXTURES_ADDRESS_INCORRECT;
		rs.AdvanceTo(address);

		//load texture wad
		uint32_t numSystemTextures;
		rs >> numSystemTextures;
		assert(numSystemTextures == GameWorld::NUM_SYSTEM_TEXTURES);
		uint32_t numTextures;
		rs >> numTextures;
		assert(numTextures == 2048);

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
			textures[i].Load(rs, pool);

		//load surface materials
		uint32_t nsurface_materials;
		rs >> nsurface_materials;
		surfaceMaterials = std::move(pool.CreateArray<SurfaceMaterial>(nsurface_materials));
		for (auto &sm : surfaceMaterials)
			sm.Load(rs);

		//load surface properties
		uint32_t nsurface_properties;
		rs >> nsurface_properties;
		surfaceProperties = std::move(pool.CreateArray<SurfaceProperty>(nsurface_properties));
		for (auto &sp : surfaceProperties)
			sp.Load(rs, pool);
	}

	rs.Close();
	return RESULT::CODE::OK;
}
