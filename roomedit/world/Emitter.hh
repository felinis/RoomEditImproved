#pragma once
#include "common.hh"
#include "sbfilesystem/ReadStream.hh"
#include "sbmemory/MemoryPool.hh"

struct Emitter
{
	char name[MAX_DRAWABLE_NAME_LENGTH];

	bool colorAddition;

	float gravity;

	int rate;
	float lifeTime;
	int timer;
	int pause;

	RGBAColor startColor;
	RGBAColor endColor;

	float startScale;
	float endScale;

	float speed;
	float speedRandom;
	float scatter;

	float areaX;
	float areaY;

	float spin;
	int imageNumber;

	int particleType;
	int numInitialParticles;

	Array<uint32_t> drawableIndices;

	float particleLength;

	bool deflectionPlaneActive;
	float deflectionPlaneReduction;

public:
	void Load(ReadStream &rs, MemoryPool &pool);
};
