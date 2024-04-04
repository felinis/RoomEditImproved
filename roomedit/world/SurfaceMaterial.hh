#pragma once
#include <stdint.h>
#include <assert.h>
#include <sbfilesystem/ReadStream.hh>
#include <sbmemory/MemoryPool.hh>

struct SurfaceMaterial
{
	union
	{
		struct
		{
			int32_t texture_index_diffuse;
			int32_t texture_index_bump;
			int32_t texture_index_gloss;
			int32_t texture_index_enviromentmap;
			int32_t texture_index_selfilluminating; //lightmap
		};
		int32_t texture_index_array[5];
	};

	bool used;
	bool clamp;
	uint8_t enviromentmap_strength;
	uint8_t selfillumination_strength; //lightmap strength

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

	void Load(ReadStream &rs)
	{
		uint16_t version;
		rs >> version;

		rs >> used;
		if (used)
		{
			if (version > 5)
				rs >> clamp;

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
				assert(numTextures == 5);
				rs.Read(&texture_index_array, (size_t)numTextures * 4);

				if (version < 5)
					rs >> enviromentmap_strength;
				if (version > 4)
					rs >> selfillumination_strength;
			}
		}
	}

	int32_t GetBaseTextureIndex() const
	{
		if (texture_index_diffuse == -1 &&
			texture_index_selfilluminating == -1 &&
			texture_index_enviromentmap == -1)
			return -1;
		else
			return texture_index_diffuse;
	}
};
