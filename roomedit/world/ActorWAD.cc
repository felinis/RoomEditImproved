/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "ActorWAD.hh"
#include <memory.h>

bool ActorWAD::CheckActorStateRootProject()
{
	for (const auto &actor : actors)
	{
		if (actor.stateRootProject == -1)
		{
			//TODO: show error "State root project undefined for actor"
			return false;
		}
	}

	return true;
}

bool ActorWAD::Load(ReadStream &rs, MemoryPool &pool)
{
	char headerName[14];
	rs >> headerName;
	if (memcmp(headerName, "ACW Actor Wad", 14) != 0)
		return false;

	rs >> version;
	assert(version == 9);

	uint32_t numActors;
	rs >> numActors;
	assert(numActors <= 30); //sanity check

	uint32_t numProjects;
	rs >> numProjects;
	assert(numProjects <= 50); //sanity check

	uint32_t numCutscenes;
	rs >> numCutscenes;
	assert(numCutscenes <= 20); //sanity check

	uint32_t numTextures;
	rs >> numTextures;
	assert(numTextures <= 200); //sanity check

	uint32_t numSurfaceMaterials;
	rs >> numSurfaceMaterials;
	assert(numSurfaceMaterials <= 200); //sanity check

	uint32_t numSurfaceProperties;
	rs >> numSurfaceProperties;
	assert(numSurfaceProperties <= 200); //sanity check

	//load actors
	actors = pool.CreateArray<Actor>(numActors);
	for (auto &a : actors)
		a.Load(rs, pool);

	//load projects
	projects = pool.CreateArray<Project>(numProjects);
	for (auto &p : projects)
		p.Load(rs, pool);

	//load cutscenes
	cutscenes = pool.CreateArray<CutScene>(numCutscenes);
	for (auto &c : cutscenes)
		c.Load(rs, pool);

	uint32_t textinfoVersion;
	rs >> textinfoVersion;

	//load textinfos and texture images
//	for (auto& p : acw)
//	{
//	}

	//load surface materials
//	surfaceMaterials = pool.CreateArray<SurfaceMaterial>(numSurfaceMaterials);
//	for (auto& sm : surfaceMaterials)
//		sm.Load(rs);

	//load surface properties
//	surfaceProperties = pool.CreateArray<SurfaceProperty>(numSurfaceProperties);
//	for (auto& sp : surfaceProperties)
//		sp.Load(rs);

	return CheckActorStateRootProject();
}
