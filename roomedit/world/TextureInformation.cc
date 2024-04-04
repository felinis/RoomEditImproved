/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "TextureInformation.hh"

void TextureInformation::Load(ReadStream &rs, MemoryPool &pool)
{
	uint32_t numFrames;
	rs >> numFrames;
	assert(numFrames <= 30); //sanity check

	frames = std::move(pool.CreateArray<Frame>(numFrames));
	for (auto &frame : frames)
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
		assert(skipped[0] >> 16);
		rs >> frame.magic; //DXT identifier

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

