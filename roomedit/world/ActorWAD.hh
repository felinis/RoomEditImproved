#pragma once
#include "sbfilesystem/ReadStream.hh"
#include "sbmemory/MemoryPool.hh"
#include "Actor.hh"
#include "SurfaceMaterial.hh"
#include "SurfaceProperty.hh"
#include <stdint.h>

struct PACKEDKEY
{
	short time;
	uint8_t tcb_index;
	uint8_t data;
	uint16_t index1;
	uint16_t index2;
	uint16_t index3;
	uint16_t index4;
};
static_assert(sizeof(PACKEDKEY) == 12);

struct QUAT
{
	float w;
	float x;
	float y;
	float z;
};
static_assert(sizeof(QUAT) == 16);

struct ANIM_POOL
{
	Array<PACKEDKEY> keys;
	Array<Vector3> vectors;
	Array<QUAT> quats;

	void Load(ReadStream &rs, MemoryPool &pool)
	{
		uint32_t smth;
		rs >> smth;
		uint32_t nkeys;
		rs >> nkeys;
		uint32_t nvectors;
		rs >> nvectors;
		uint32_t nquats;
		rs >> nquats;

		//		AddNewKeys(nkeys, true);
		//		AddNewVectors(nvectors, true);
		//		AddNewQuats(nquats, true);

		keys = std::move(pool.CreateArray<PACKEDKEY>(nkeys));
		rs.Read(keys.Data(), nkeys * sizeof(PACKEDKEY)); //read directly

		vectors = std::move(pool.CreateArray<Vector3>(nvectors));
		rs.Read(vectors.Data(), nvectors * sizeof(Vector3)); //read directly

		quats = std::move(pool.CreateArray<QUAT>(nquats));
		rs.Read(quats.Data(), nquats * sizeof(QUAT)); //read directly
	}
};

struct ANIM_SET
{
	int prj_id;
	int spec_id;
};
static_assert(sizeof(ANIM_SET) == 8);

struct EXPR
{
	uint8_t type;
//	char padding;
//	uint16_t flag;
};

struct COND
{
	Array<EXPR> exprs;

	void Load(ReadStream &rs, MemoryPool &pool)
	{
		uint32_t numExpressions;
		rs >> numExpressions;
		assert(numExpressions < 100);
		exprs = pool.CreateArray<EXPR>(numExpressions);
		for (auto &expr : exprs)
			rs >> expr.type;
	}
};

struct TRANGE
{
	float st;
	float et;
};

struct LINK_KEY
{
	TRANGE tr;
	bool dynamic;
	bool dyn_frames;
	short range_id;
};
static_assert(sizeof(LINK_KEY) == 12);

struct LINK
{
	COND cond;
	int state;
	ANIM_SET linkto;
	ANIM_SET via_link;
	Array<LINK_KEY> keys;

	void Load(ReadStream &rs, MemoryPool &pool)
	{
		cond.Load(rs, pool);
		rs >> state;
		rs >> linkto;
		rs >> via_link; //TODO: PS2 code different from PC code!!!

		uint32_t numKeys;
		rs >> numKeys;
		keys = pool.CreateArray<LINK_KEY>(numKeys);
		rs.Read(keys.Data(), numKeys * sizeof(LINK_KEY)); //read directly
	}
};

struct RANGE_KEY
{
	TRANGE tr;
};
static_assert(sizeof(RANGE_KEY) == 8);

struct COMMAND
{
	COND cond;
	enum COMMAND_TYPE
	{
		COMTYPE_IK_START = 0,
		COMTYPE_IK_END = 1,
		COMTYPE_EXTRA_ROTATION = 2,
		COMTYPE_SPLIT_ANIM = 3,
		COMTYPE_JOIN_ANIM = 4,
		COMTYPE_HKY_INSERT = 5,
		COMTYPE_HKY_REMOVE = 6,
		COMTYPE_ACTION = 7,
		COMTYPE_HKY_MORPH = 8,
		COMTYPE_HKY_MORPH_REVERSE = 9,
		COMTYPE_SOUND = 10,
		COMTYPE_JOIN_HOLD = 11,
		COMTYPE_TEXTURE_PROFILE = 12,
		COMTYPE_CUTSCENE = 13,
		COMTYPE_INVALID = 14
	};
	COMMAND_TYPE type;
	struct COMMAND_KEY
	{
		float time;
		bool active;

		void Load(ReadStream &rs, MemoryPool &pool)
		{
			rs >> active;
			rs.AdvanceBy(3);
			rs >> time;
		}
	};
	Array<COMMAND_KEY> keys;

	void Load(ReadStream &rs, MemoryPool &pool)
	{

		uint32_t smth;
		rs >> smth;
		cond.Load(rs, pool);
		rs >> type;

		uint32_t numKeys;
		rs >> numKeys;
		keys = pool.CreateArray<COMMAND_KEY>(numKeys);
		for (auto &key : keys)
		{
			uint32_t smth;
			rs >> smth;
			key.Load(rs, pool);
		}
	}
};

struct POSKEY
{
	float time;
	float oodt;
	short tcb_index;
	uint8_t field3_0xa;
	uint8_t field4_0xb;
	Vector3 p;
	Vector3 c1;
	Vector3 c2;
	Vector3 c3;
};
static_assert(sizeof(POSKEY) == 60);

struct ROTKEY
{
	float time;
	float oodt;
	short tcb_index;
	uint8_t linearity;
	uint8_t padding;
	QUAT q;
	QUAT q0;
	QUAT q1;
};
static_assert(sizeof(ROTKEY) == 60);

struct ANIM
{
	Array<POSKEY> pos;
	Array<ROTKEY> rot;

	void Load(ReadStream &rs, MemoryPool &pool)
	{
		uint32_t numPositionKeys;
		rs >> numPositionKeys;
		pos = pool.CreateArray<POSKEY>(numPositionKeys);
		rs.Read(pos.Data(), numPositionKeys * sizeof(POSKEY)); //read directly


		uint32_t numRotationKeys;
		rs >> numRotationKeys;
		rot = pool.CreateArray<ROTKEY>(numRotationKeys);
		rs.Read(rot.Data(), numRotationKeys * sizeof(ROTKEY)); //read directly

		//skip some old stuff, unused by the game
		uint32_t local_4;
		rs >> local_4;
		if (local_4)
		{
			rs.AdvanceBy(local_4 * sizeof(POSKEY));
		}
	}
};

struct ANIM_PACKED
{
	int numPositionKeys;
	PACKEDKEY *pos;
	int numRotationKeys;
	PACKEDKEY *rot;

	void Load(ReadStream &rs, MemoryPool &pool, ANIM_POOL *animPool)
	{
		uint32_t keyIndex;

		rs >> numPositionKeys;
		rs >> keyIndex;
		pos = &animPool->keys[keyIndex];

		rs >> numRotationKeys;
		rs >> keyIndex;
		rot = &animPool->keys[keyIndex];
	}
};

struct TCB
{
	float t;
	float c;
	float b;
	float easeTo;
	float easeFrom;
};

struct FLOATKEY
{
	float time;
	float oodt;
	short tcb_index;
	float val;
	float c1;
	float c2;
	float c3;
};

struct SPECIAL_ANIM
{
	int id;
};

constexpr uint32_t ANIMATION_VERSION = 7;
struct ANIMATION
{
	ANIM_POOL *pool;
	bool packed;

	//only one of these 2 is loaded
	union
	{
		Array<ANIM> bones;
		Array<ANIM_PACKED> packed_bones;
	};

	float nframes;

	//only one of these 2 is loaded
	union
	{
		ANIM *velocity;
		ANIM_PACKED *packed_velocity;
	};

	Array<TCB> tcbs;
	uint32_t tcb_array_size;

	bool hips_absolute;
	Array<SPECIAL_ANIM> specials;

	bool precalc;

	bool cylinder_height_set;
	float cylinder_height;

	bool Load(ReadStream &rs, MemoryPool &pool, ANIM_POOL *animPool)
	{
		this->pool = animPool;
		rs >> packed;
		uint32_t numBones;
		rs >> numBones;
		assert(numBones < 60);

		uint32_t version;
		rs >> version;
		assert(version == 2);

		if (!packed)
		{
			bones = pool.CreateArray<ANIM>(numBones);
			for (auto &bone : bones)
				bone.Load(rs, pool);
		}
		else
		{
			packed_bones = pool.CreateArray<ANIM_PACKED>(numBones);
			for (auto &bone : packed_bones)
				bone.Load(rs, pool, animPool);
		}

		rs >> nframes;

		uint32_t ntcbs;
		rs >> ntcbs;
		tcb_array_size = ntcbs;
		tcbs = pool.CreateArray<TCB>(ntcbs);
		rs.Read(tcbs.Data(), ntcbs * sizeof(TCB)); //read directly

		rs >> hips_absolute;

		uint32_t nspecials;
		rs >> nspecials;
		specials = pool.CreateArray<SPECIAL_ANIM>(nspecials);
		for (auto &spec : specials)
		{
			rs >> spec.id;
			uint32_t type;
			rs >> type;
			switch (type)
			{
			case 0:
			{
				ANIM anim;
				anim.Load(rs, pool);
				break;
			}
			case 1:
			{
				struct ANIM_UV
				{
					Array<FLOATKEY> u;
					Array<FLOATKEY> v;

					void Load(ReadStream &rs, MemoryPool &pool)
					{
						uint32_t numU;
						rs >> numU;
						u = pool.CreateArray<FLOATKEY>(numU);
						rs.Read(u.Data(), numU * sizeof(FLOATKEY)); //read directly

						uint32_t numV;
						rs >> numV;
						v = pool.CreateArray<FLOATKEY>(numV);
						rs.Read(u.Data(), numV * sizeof(FLOATKEY)); //read directly
					}
				};
				ANIM_UV animUV;
				animUV.Load(rs, pool);
				break;
			}
			case 2:
			{
				struct ANIM_CAMERA
				{
					Array<POSKEY> pos;
					Array<POSKEY> target;
					Array<FLOATKEY> roll;
					Array<FLOATKEY> fov;

					void Load(ReadStream &rs, MemoryPool &pool)
					{
						uint32_t numPositionKeys;
						rs >> numPositionKeys;
						pos = pool.CreateArray<POSKEY>(numPositionKeys);
						rs.Read(pos.Data(), numPositionKeys * sizeof(POSKEY)); //read directly

						uint32_t numTargetKeys;
						rs >> numTargetKeys;
						target = pool.CreateArray<POSKEY>(numTargetKeys);
						rs.Read(target.Data(), numTargetKeys * sizeof(POSKEY)); //read directly

						uint32_t numRollKeys;
						rs >> numRollKeys;
						roll = pool.CreateArray<FLOATKEY>(numRollKeys);
						rs.Read(roll.Data(), numRollKeys * sizeof(FLOATKEY)); //read directly

						uint32_t numFOVKeys;
						rs >> numFOVKeys;
						fov = pool.CreateArray<FLOATKEY>(numFOVKeys);
						rs.Read(fov.Data(), numFOVKeys * sizeof(FLOATKEY)); //read directly
					}
				};
				ANIM_CAMERA animCamera;
				animCamera.Load(rs, pool);
				break;
			}
			default:
				assert(0);
				return false;
			}
		}

		rs >> cylinder_height_set;
		rs >> cylinder_height;

		//has_vel is true

		if (!packed)
		{
			velocity = pool.Allocate<ANIM>();
			velocity->Load(rs, pool);
		}
		else
		{
			packed_velocity = pool.Allocate<ANIM_PACKED>();
			packed_velocity->Load(rs, pool, animPool);
		}

		return true;
	}
};

struct ALT_ANIM
{
	bool original;
	COND cond;
	ANIMATION *anim;

	bool Load(ReadStream &rs, MemoryPool &pool, ANIM_POOL *animPool)
	{
		rs >> original;

		cond.Load(rs, pool);

		anim = pool.Allocate<ANIMATION>();
		return anim->Load(rs, pool, animPool);
	}
};

struct ANIM_SPEC
{
	int state;
	uint8_t prop_to_bind;
	char field2_0x5;
	char field3_0x6;
	char field4_0x7;
	ANIM_SET jumpto;
	float jumpto_time;
	Array<LINK> links;
	Array<RANGE_KEY> range;
	Array<COMMAND> commands;
	Array<ALT_ANIM> altAnims;
	uint8_t type;
	uint8_t dynlink_dest_alt_anim;
	char field13_0x26;
	char field14_0x27;

	bool Load(ReadStream &rs, MemoryPool &pool, ANIM_POOL *animPool)
	{
		uint32_t smth;
		rs >> smth;
		switch (smth)
		{
		case 1:
		{
			assert(0);
			//COND constructor
			//COND destructor
			rs >> state;
//			alt_anim[0].anim.Load(rs, pool, nullptr);
			rs >> prop_to_bind;
			rs >> jumpto;
			rs >> jumpto_time;

//			OLD_ANIM::Load(&local_2c0, h);
//			convert_old_anim(&local_2c0, pAVar1, pAVar2);

			uint32_t numRangeKeys;
			rs >> numRangeKeys;
			for (auto &rangeKey : range)
			{
				RANGE_KEY rk;
				rs >> rk;
				uint32_t y;
				rs >> y;
				rs >> y;
			}

			uint32_t numLinks;
			rs >> numLinks;
//			for (auto &link : links)
//				link.Load(rs, pool);

			uint32_t temp;
			rs >> temp;
			rs >> temp;
			break;
		}
		case 7:
		{
			rs >> state;
			rs >> prop_to_bind;
			rs >> jumpto;
			rs >> jumpto_time;

			uint32_t numRanges;
			rs >> numRanges;
			range = pool.CreateArray<RANGE_KEY>(numRanges);
			rs.Read(range.Data(), numRanges * sizeof(RANGE_KEY)); //read directly

			uint32_t numLinks;
			rs >> numLinks;
			links = pool.CreateArray<LINK>(numLinks);
			for (auto &link : links)
				link.Load(rs, pool);

			uint32_t numCommands;
			rs >> numCommands;
			commands = pool.CreateArray<COMMAND>(numCommands);
			for (auto &command : commands)
				command.Load(rs, pool);

			uint32_t numAltAnims;
			rs >> numAltAnims;
			altAnims = pool.CreateArray<ALT_ANIM>(numAltAnims);
			for (auto &altAnim : altAnims)
				altAnim.Load(rs, pool, animPool);

			rs >> type;

			break;
		}
		default:
		{
			return false;
		}
		}

		return true;
	}
};

struct SPLIT_DEF
{
	uint32_t splitID;
	Array<uint8_t> bones;

	void Load(ReadStream &rs, MemoryPool &pool)
	{
		uint32_t local_8;
		rs >> local_8;
		if (local_8 == 1)
		{
			rs >> splitID;
			uint32_t numBones;
			rs >> numBones;
			bones = std::move(pool.CreateArray<uint8_t>(numBones));
			rs.Read(bones.Data(), numBones * sizeof(uint8_t)); //read directly
		}
	}
};

struct ActorWAD
{
	uint32_t version;
	Array<Actor> actors;

	struct Project
	{
		Array<ANIM_SPEC> specs;
		ANIM_POOL *animPool;
		Array<SPLIT_DEF> splits;

		uint32_t projectID;

		void Load(ReadStream &rs, MemoryPool &pool)
		{
			int32_t numSpecs;
			rs >> numSpecs;
			uint32_t local_20;
			if (numSpecs < 0)
			{
				numSpecs = -numSpecs;
				rs >> local_20;
			}
			else
				local_20 = 1;

			switch (local_20)
			{
			case 4:
			{
				bool local_1;
				rs >> local_1;
				if (local_1)
				{
					animPool = pool.Allocate<ANIM_POOL>();
					animPool->Load(rs, pool);
				}
				else
					animPool = nullptr;

				specs = pool.CreateArray<ANIM_SPEC>(numSpecs);
				for (auto &spec : specs)
					spec.Load(rs, pool, animPool);

				uint32_t numSplitDefs;
				rs >> numSplitDefs;
				splits = pool.CreateArray<SPLIT_DEF>(numSplitDefs);
				for (auto &split : splits)
					split.Load(rs, pool);

				rs >> projectID;
				break;
			}
			default:
			{
				assert(0);
				break;
			}
			}
		}
	};
	Array<Project> projects;

	struct CutScene
	{
		void Load(ReadStream &rs, MemoryPool &pool)
		{
			uint32_t s;
			rs >> s;

			//TODO
		}
	};
	Array<CutScene> cutscenes;

	Array<SurfaceMaterial> surfaceMaterials;
	Array<SurfaceProperty> surfaceProperties;

	bool CheckActorStateRootProject();
	bool Load(ReadStream &rs, MemoryPool &pool);
};
