#pragma once
#include "sbfilesystem/ReadStream.hh"
#include "sbmemory/MemoryPool.hh"
#include <stdint.h>

class TextureInformation
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
		uint8_t *data;
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

public:
	void Load(ReadStream &rs, MemoryPool &pool);

	inline bool HasFrames() const
	{
		return frames.Count() != 0;
	}

	inline const Frame &GetFrame(uint32_t index) const
	{
		return frames[index];
	}

	inline Usage GetTextureUsage() const
	{
		return usage;
	}

	inline bool IsTransparent() const
	{
		return transparent;
	}

	inline bool HasMipMaps() const
	{
		return allowMipMaps;
	}

	inline bool IsLightMapped() const
	{
		return selfIlluminated;
	}
};
