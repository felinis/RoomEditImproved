/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "Actor.hh"
#include "ObjectProperties.hh"
#include <assert.h>

static bool ReadMeshes(Array<Mesh> &meshes, ReadStream &rs, MemoryPool &pool)
{
	uint32_t numMeshes;
	rs >> numMeshes;
	if (numMeshes > 256) //sanity check
		return false;

	meshes = std::move(pool.CreateArray<Mesh>(numMeshes));
	for (auto &mesh : meshes)
		mesh = ReadMesh(rs, pool);

	return true;
}

bool Actor::Model::Load(ReadStream &rs, MemoryPool &pool)
{
	uint32_t type;
	rs >> type;
	switch (type)
	{
	case 5:
	{
		uint32_t version;
		rs >> version;
		assert(version == 72);

		uint32_t someValue;
		rs >> someValue;
		rs >> skinned;
		rs >> id;

		if (!ReadMeshes(meshes, rs, pool))
			return false;

		uint32_t numBones;
		rs >> numBones;
		hkyFlag = pool.CreateArray<uint32_t>(numBones);
		rs.Read(hkyFlag.Data(), sizeof(uint32_t) * numBones);

		uint32_t numSkinGroups;
		rs >> numSkinGroups;
		skinData = pool.CreateArray<SkinData>(numSkinGroups);
		for (auto &sd : skinData)
			sd.Load(rs, pool);

		collide.Load(rs, pool, numBones);
		bool hasMorph;
		rs >> hasMorph;
		if (hasMorph)
		{
			morph = pool.Allocate<ModelMorph>(1);
			morph->Load(rs, pool);
		}
		else
			morph = nullptr;

		uint32_t ntexture_id2index;
		rs >> ntexture_id2index;
		texture_id2index = pool.CreateArray<uint64_t>(ntexture_id2index);
		rs.Read(texture_id2index.Data(), sizeof(uint64_t) * ntexture_id2index);

		bool hasRemap;
		rs >> hasRemap;
		if (hasRemap)
		{
			remap = pool.Allocate<PRRemap>(1);
			remap->Load(rs, pool);
		}
		else
			remap = nullptr;

		break;
	}
	default:
		return false;
	}

	return true;
}

bool Actor::LoadModel(ReadStream &rs, MemoryPool &pool)
{
	uint32_t num;
	rs >> num;
	model = pool.CreateArray<Model>(num);
	for (auto &m : model)
		m.Load(rs, pool);
	return true;
}

bool Actor::LoadBodyLoc(ReadStream &rs, uint32_t versionBodyLoc)
{
	uint32_t numBodyLocs;
	rs >> numBodyLocs;
	for (uint32_t i = 0; i < numBodyLocs; i++)
	{
		switch (versionBodyLoc - 1)
		{
		case 0:
			rs.AdvanceBy(4);
			rs.AdvanceBy(12);
			rs.AdvanceBy(4);
			rs.AdvanceBy(4);
			rs.AdvanceBy(4);
			break;
		case 1:
		case 3:
			rs.AdvanceBy(4);
			rs.AdvanceBy(4);
			rs.AdvanceBy(12);
			rs.AdvanceBy(12);
			rs.AdvanceBy(4);
			break;
		case 2:
			rs.AdvanceBy(4);
			rs.AdvanceBy(4);
			rs.AdvanceBy(12);
			rs.AdvanceBy(16);
			//quat to matrix
			//matrix to euler
			rs.AdvanceBy(4);
			break;
		default:
			return false;
		}
	}

	return true;
}

static void sub_4D1580(ReadStream &rs)
{
	uint32_t t;
	rs >> t;
	if (t == 1)
	{
		rs.AdvanceBy(4);
		uint32_t sz;
		rs >> sz;
		for (uint32_t i = 0; i < sz; i++)
		{
			rs.AdvanceBy(4);
			rs.AdvanceBy(1);
			rs.AdvanceBy(4);
			rs.AdvanceBy(4);
			rs.AdvanceBy(4);
			rs.AdvanceBy(4);
		}
	}
}

static void sub_4D14D0(ReadStream &rs)
{
	uint32_t s;
	rs >> s;
	for (uint32_t i = 0; i < s; i++)
	{
		sub_4D1580(rs);
	}
}

bool Actor::Load(ReadStream &rs, MemoryPool &pool)
{
	uint32_t type;
	rs >> type;
	switch (type)
	{
	case 100:
	{
		rs >> currModel;
		uint32_t numFlags;
		rs >> numFlags;
		assert(0); //UNFINISHED
		break;
	}
	case 109:
	{
		rs >> objectId;
		if (!LoadModel(rs, pool))
			return false;

		rs >> currModel;

		uint32_t numFlags;
		rs >> numFlags;
		flags = pool.CreateArray<uint16_t>(numFlags);
		rs.Read(flags.Data(), sizeof(uint16_t) * numFlags);

		uint32_t versionBodyLoc;
		rs >> versionBodyLoc;
		LoadBodyLoc(rs, versionBodyLoc);

		rs >> bindPose;
		rs >> initAnim;
		rs >> velocityRatio;
		rs >> stateRootProject;

		uint32_t numBones;
		rs >> numBones;
		bone_rel = pool.CreateArray<uint32_t>(numBones);
		rs.Read(bone_rel.Data(), sizeof(uint32_t) * numBones);

		sub_4D14D0(rs);

		bool readd;
		rs >> readd;
		if (readd)
		{
			uint32_t version;
			rs >> version;
			assert(version == 72);
			OBJECT_PROPERTIES properties;
			properties.Load(rs, pool);
		}

		rs >> hasShadow;
		break;
	}
	default:
		assert(0); //UNFINISHED
	}

	return true;
}
