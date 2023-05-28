#pragma once
#include "sbMemory/MemoryPool.h"
#include "sbfilesystem/ReadStream.h"
#include <assert.h>

struct WorldHeader
{
	uint32_t version;
	int32_t levelId;
	uint32_t numPlayers;
	uint32_t flags;
	char levelName[4];
	char nextLevelName[4];

	WorldHeader():
		version(72),
		levelId(-1),
		numPlayers(4),
		flags(0),
		levelName(),
		nextLevelName()
	{}
};
static_assert(sizeof(WorldHeader) == 24);

struct Vector3
{
	float x, y, z;
};
static_assert(sizeof(Vector3) == 12);

struct AINetwork
{

};

struct Light
{
	enum class Type: uint32_t
	{
		LIGHT,
		FOG,
		DYNAMIC_LIGHT,
		SPOTLIGHT,
		FOGPLANE,
		DYNAMIC_SPOT
	};
	Type type;
	Vector3 position;
	int32_t color[3]; //rgb
	float intensity;
	float falloff;
	float falloff_cos;
	float hotspot;
	float hotspot_cos;
	Vector3 direction;
	float ambient;
};
static_assert(sizeof(Light) == 64);

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

		Hull():
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

	Face():
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
	int32_t flags;

	uint32_t numVerts;
	Vector3 *positions;
	Vector3 *normals;
	Vector3 minExtent;
	Vector3 maxExtent;

	Array<Corner> corners;
	Array<Face> faces;

	Mesh():
		flags(0),
		numVerts(0),
		positions(nullptr),
		normals(nullptr),
		minExtent(),
		maxExtent()
	{}
};

struct Trigger
{
	Vector3 min;
	Vector3 max;
};

struct Room
{
	float scale;
	Vector3 position;

	//sound data
	int32_t sfxAmbient;
	struct SFX_ENVIRONMENT_DATA
	{
		uint16_t type;
		uint8_t delay;
		uint8_t feedback;
	};
	SFX_ENVIRONMENT_DATA sfxEnvironmentData;

	Array<Light *> lights;
	Array<Trigger> triggers;

	Mesh mesh;
};

struct RGBAColor
{
	uint8_t r, g, b, a;

	RGBAColor(uint32_t color):
		r(color & 0xFF000000),
		g(color & 0x00FF0000),
		b(color & 0x0000FF00),
		a(color & 0x000000FF)
	{}
};

struct TextureInformation
{
	struct Frame
	{
		uint16_t width;
		uint16_t padding;
		uint32_t height;
		uint32_t pixelSize;
		uint32_t dword18;
		uint32_t magic;
		uint32_t size;
		uint8_t* data;
	};
	Array<Frame> frames;

	enum class Usage : uint16_t
	{
		NONE = 0,
		DIFFUSE = 1,
		LIGHT = 2,
		BUMP = 3,
		GLOSS = 4,
		ENVIROMENTMAP = 5,
		SPRITE = 6,
		SELFILLUMINATING = 7,
		TEXT = 8,
		IMAGE = 9,
		END = 10
	};
	Usage usage;

	bool transparent;
	bool allowMipMaps;
	bool selfIlluminated; //lightmapped

	TextureInformation():
		usage(Usage::NONE),
		transparent(false),
		allowMipMaps(false),
		selfIlluminated(false)
	{}

	void Load(ReadStream& rs, MemoryPool& pool)
	{
		uint32_t numFrames;
		rs >> numFrames;
		assert(numFrames <= 30); //sanity check

		frames = std::move(pool.CreateArray<Frame>(numFrames));
		for (auto& frame : frames)
		{
			uint32_t v;
			rs >> v;
			frame.width = (unsigned short)v;
			rs >> frame.height;
			rs >> frame.pixelSize;
			rs >> frame.dword18;

			uint16_t shifted = v >> 16;
			uint32_t size;
			if (shifted <= 0)
				size = frame.width * frame.height * frame.pixelSize;
			else
				rs >> size;
			frame.size = size;

			uint32_t skipped[7];
			rs >> skipped;
			if (skipped[0] >> 16)
			{
				rs >> frame.magic;
#if 0
				switch (magic)
				{
				default:
				case '1TXD':
					frame.format = TEXTURE_FORMAT::BC1_UNorm;
					break;
				case '3TXD':
					frame.format = TEXTURE_FORMAT::BC2_UNorm;
					break;
				case '5TXD':
					frame.format = TEXTURE_FORMAT::BC3_UNorm;
					break;
				}
#endif
			}
			else
				assert(0);

			frame.data = pool.Allocate<uint8_t>(size);
			rs.Read(frame.data, size);

			if (shifted > 1)
			{
				bool readAgain;
				rs >> readAgain;
				assert(!readAgain);
			}
		}

		//if this texture is valid, read more stuff
		if (frames.Count())
		{
			rs >> usage;
			rs >> transparent;
			rs >> allowMipMaps;
			rs >> selfIlluminated;
		}
	}
};

struct SurfaceMaterial
{
	union
	{
		struct
		{
			int texture_index_diffuse;
			int texture_index_bump;
			int texture_index_gloss;
			int texture_index_enviromentmap;
			int texture_index_selfilluminating; //lightmap
		};
		int texture_index_array[5];
	};
	
	unsigned char used;
	unsigned char clamp;
	unsigned char enviromentmap_strength;
	unsigned char selfillumination_strength; //lightmap strength

	SurfaceMaterial()
	{
		texture_index_diffuse = -1;
		texture_index_bump = -1;
		texture_index_gloss = -1;
		texture_index_enviromentmap = -1;
		texture_index_selfilluminating = -1;
		used = false;
		clamp = false;
		enviromentmap_strength = 128;
		selfillumination_strength = 0;
	}

	void Load(ReadStream& rs)
	{
		uint16_t version;
		rs >> version;

		bool used;
		rs >> used;
		if (used)
		{
			if (version > 5)
				rs.AdvanceBy(1);

			if (version < 3)
			{
				rs >> texture_index_diffuse;
				rs >> texture_index_bump;
				rs >> texture_index_gloss;
				rs >> texture_index_enviromentmap;

				if (version > 1)
					rs >> texture_index_selfilluminating;
			}

			if (version > 2)
			{
				uint32_t numTextures;
				rs >> numTextures;
				assert(numTextures <= 5);
				rs.Read(&texture_index_array, (size_t)numTextures * 4);

				if (version < 5)
					rs.AdvanceBy(1);
				if (version > 4)
					rs.AdvanceBy(1);
			}
		}
	}

	int GetBaseTextureIndex() const
	{
		if (texture_index_diffuse == -1 &&
			texture_index_selfilluminating == -1 &&
			texture_index_enviromentmap == -1)
			return -1;
		else
			return texture_index_diffuse;
	}
};

struct SurfaceProperty
{
	Array<uint16_t> pmaterials;
	uint16_t selected_material;
	bool enable_modulate;
	uint8_t modulate_red;
	uint8_t modulate_green;
	uint8_t modulate_blue;
	uint8_t modulate_alpha;
	bool enable_offset_uv;
	float offset_u;
	float offset_v;
	bool unique;
	uint16_t surface_type;

	SurfaceProperty()
	{
		selected_material = 0;

		enable_modulate = false;
		modulate_red = 0xFF;
		modulate_green = 0xFF;
		modulate_blue = 0xFF;
		modulate_alpha = 0xFF;

		enable_offset_uv = false;
		offset_u = 0.0f;
		offset_v = 0.0f;

		unique = false;
		surface_type = 0;
	}

	void Load(ReadStream& rs, MemoryPool& pool)
	{
		uint16_t a;
		rs >> a;
		uint16_t nmaterials;
		rs >> nmaterials;
		if (nmaterials)
		{
			pmaterials = pool.CreateArray<uint16_t>(nmaterials);
			for (uint16_t i = 0; i < nmaterials; i++)
				rs >> pmaterials[i];
			rs >> selected_material;
		}

		rs >> enable_modulate;
		if (a > 2 || enable_modulate)
		{
			rs >> modulate_red;
			rs >> modulate_green;
			rs >> modulate_blue;
			rs >> modulate_alpha;
		}

		rs >> enable_offset_uv;
		if (a > 2 || enable_offset_uv)
		{
			rs >> offset_u;
			rs >> offset_v;
		}

		if (a > 1)
			rs >> unique;
		if (a > 3)
			rs >> surface_type;
	}
};

class GameWorld
{
	MemoryPool pool;

public:
	GameWorld():
		fogColor(0)
	{}

	WorldHeader worldHeader;
	Array<int32_t> drawables;
	Array<AINetwork> aiNetworks;
	Array<Light> lights;
	Array<Room> rooms;
	Array<Room> reflectors;

	RGBAColor fogColor;

//	Array<Mesh> meshes;
	Array<TextureInformation> textures;
	Array<SurfaceMaterial> surfaceMaterials;
	Array<SurfaceProperty> surfaceProperties;

	/*
	*	Loads a game world from a level file and uses a memory pool to store allocated data.
	*/
	bool Load(const char* filePath);
	void Unload();

	static constexpr int32_t NUM_SYSTEM_TEXTURES = 5;
	int GetBaseTextureIndex(int index_surface_property) const
	{
		index_surface_property -= NUM_SYSTEM_TEXTURES;

		int textureIndex = -1;
		int materialIndex;

		if ((index_surface_property < 0) || (0x3ff < index_surface_property))
			materialIndex = -1;
		else if (surfaceProperties[index_surface_property].pmaterials.Count() == 0)
			materialIndex = -1;
		else
		{
			const SurfaceProperty& surfProp = surfaceProperties[index_surface_property];
			materialIndex = surfProp.pmaterials[surfProp.selected_material] - NUM_SYSTEM_TEXTURES;
		}
		if (materialIndex != -1)
			textureIndex = surfaceMaterials[materialIndex].GetBaseTextureIndex() - NUM_SYSTEM_TEXTURES;
		return textureIndex;
	}
};
