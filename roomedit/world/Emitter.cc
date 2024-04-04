/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "Emitter.hh"

void Emitter::Load(ReadStream &rs, MemoryPool &pool)
{
	LoadDrawableName(rs, name);

	rs >> colorAddition;
	rs >> gravity;
	int buffer;
	rs >> buffer;
	rs >> rate;
	rs >> lifeTime;
	rs >> timer;
	rs >> pause;
	rs >> startColor;
	rs >> endColor;
	rs >> startScale;
	rs >> endScale;
	rs >> speed;
	rs >> scatter;
	rs >> areaX;
	rs >> areaY;
	rs >> spin;
	uint32_t thing2;
	rs >> thing2;

	rs >> imageNumber;

	rs >> particleType;

	rs >> numInitialParticles;

	uint32_t numDrawableIndices;
	rs >> numDrawableIndices;
	drawableIndices = std::move(pool.CreateArray<uint32_t>(numDrawableIndices));
	rs.Read(drawableIndices.Data(), numDrawableIndices * sizeof(uint32_t)); //read directly

	rs >> particleLength;
	rs >> deflectionPlaneActive;
	uint32_t a;
	rs >> a;
	Vector3 b;
	rs >> b;
	rs >> deflectionPlaneReduction;
	rs >> speedRandom;
}
