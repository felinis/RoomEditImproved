#pragma once
#include <sbfilesystem/ReadStream.hh>
#include <sbmemory/MemoryPool.hh>
#include <stdint.h>

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

	void Load(ReadStream &rs, MemoryPool &pool)
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
