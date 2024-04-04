/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "common.hh"
#include <assert.h>
#include <math.h>

//Eden uses a left-handed coordinate system (X right, Y backwards, Z up),
//but we are using right-handed (X right, Y forward, Z up), so just invert Y
void ConvertHandedness(Vector3 &in)
{
	in.y = -in.y;
}

void ConvertRotation(Vector3 &in)
{
	in.x = -in.x;
	in.z = -in.z;
}

void RecomputeExtents(Vector3 &minExtent, Vector3 &maxExtent)
{
	minExtent.x = fminf(minExtent.x, maxExtent.x);
	minExtent.y = fminf(minExtent.y, maxExtent.y);
	minExtent.z = fminf(minExtent.z, maxExtent.z);
	maxExtent.x = fminf(minExtent.x, maxExtent.x);
	maxExtent.y = fminf(minExtent.y, maxExtent.y);
	maxExtent.z = fminf(minExtent.z, maxExtent.z);
}

void LoadDrawableName(ReadStream &rs, char *name)
{
	uint32_t nameLength;
	rs >> nameLength;
	assert(nameLength < MAX_DRAWABLE_NAME_LENGTH);
	rs.Read(name, nameLength);
}
