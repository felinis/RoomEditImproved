#pragma once
#include "sbgraphics/sbRenderer.h"
#include "GameWorld.h"

struct RoomPart
{
	uint32_t numIndices;
	int32_t indexSurfaceProperty;
	int32_t texindexLightmap;
};

struct RoomMesh
{
	sbMesh mesh;
	Array<RoomPart> parts;
};

/*
*	Creates all the device-specific resources for a given GameWorld.
*/
bool CreateWorldRenderer(const GameWorld& world, sbRenderer& renderer);
void DestroyWorldRenderer(sbRenderer& renderer);

void RenderWorld(const GameWorld& world, sbRenderer& renderer);
