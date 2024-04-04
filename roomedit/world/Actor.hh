#include "sbfilesystem/ReadStream.hh"
#include "sbmemory/MemoryPool.hh"
#include "Mesh.hh"
#include "Vector3.hh"

struct Actor
{
	int32_t objectId; //index of actor?
	struct Model
	{
		bool skinned;
		uint32_t id;
		Array<Mesh> meshes;
		Array<uint32_t> hkyFlag;

		struct SkinData
		{
			uint32_t vert_index;
			uint32_t nverts;
			Array<float> weights;
			Vector3 child_trans;
			Vector3 parent_trans;

			void Load(ReadStream &rs, MemoryPool &pool)
			{
				rs >> vert_index;
				rs >> nverts;
				uint32_t ndeformable;
				rs >> ndeformable;
				weights = pool.CreateArray<float>(ndeformable);
				rs.Read(weights.Data(), sizeof(float) * ndeformable); //read directly
				rs >> child_trans;
				rs >> parent_trans;
			}
		};
		Array<SkinData> skinData;

		struct HKYCollide
		{
			float radius;
			float cylinderRadius;
			float cylinderHeight;
			Array<float> boneRadius;

			void Load(ReadStream &rs, MemoryPool &pool, uint32_t val)
			{
				uint32_t a;
				rs >> a;
				switch (a)
				{
				case 1:
				{
					Vector3 v1;
					rs >> v1;
					rs >> radius;
					boneRadius = pool.CreateArray<float>(val);
					rs.Read(boneRadius.Data(), sizeof(float) * val); //read directly
					cylinderRadius = 50.0f;
					cylinderHeight = 100.0f;
					break;
				}
				case 2:
					rs >> radius;
					boneRadius = pool.CreateArray<float>(val);
					rs.Read(boneRadius.Data(), sizeof(float) * val); //read directly
					cylinderRadius = 50.0f;
					cylinderHeight = 100.0f;
					break;
				case 3:
					rs >> radius;
					boneRadius = pool.CreateArray<float>(val);
					rs.Read(boneRadius.Data(), sizeof(float) * val); //read directly
					rs >> cylinderRadius;
					rs >> cylinderHeight;
					break;
				default:
					break;
				}
			}
		};
		HKYCollide collide;

		struct ModelMorph
		{
			int32_t topologyKey;
			uint32_t nverts;
			Array<uint16_t> model2skin;
			Array<uint16_t> skin2model;

			void Load(ReadStream &rs, MemoryPool &pool)
			{
				uint32_t a;
				rs >> a;
				if (a == 1)
				{
					rs >> topologyKey;
					rs >> nverts;
					model2skin = pool.CreateArray<uint16_t>(nverts);
					rs.Read(model2skin.Data(), sizeof(uint16_t) * nverts);
					skin2model = pool.CreateArray<uint16_t>(nverts);
					rs.Read(skin2model.Data(), sizeof(uint16_t) * nverts);
				}
			}
		};
		ModelMorph *morph;

		Array<uint64_t> texture_id2index;

		struct PRRemap
		{
			Array<uint8_t> unused;
			int level;
			int index;
			Array<uint16_t> remap;
			struct PRTable
			{
				int size;
				int nindices;
				int nlevels;
				int size_vtable;
				Array<uint16_t> table;
				Array<uint16_t> vtable;

				void Load(ReadStream &rs, MemoryPool &pool)
				{
					uint32_t buffer;
					rs >> buffer;
					if (buffer == 1)
					{
						rs >> size;
						rs >> nindices;
						rs >> nlevels;
						table = pool.CreateArray<uint16_t>(size);
						rs.Read(table.Data(), sizeof(uint16_t) * size);
						rs >> size_vtable;
						vtable = pool.CreateArray<uint16_t>(size_vtable);
						rs.Read(vtable.Data(), sizeof(uint16_t) * size_vtable);
					}
				}
			};
			PRTable *reduce;
			int size_unused;
			int size_remap;
			float falloff_near_size;
			float falloff_far_size;
			float falloff_power;

			void Load(ReadStream &rs, MemoryPool &pool)
			{
				uint32_t buffer;
				rs >> buffer;
				if (buffer == 1)
				{
					rs >> size_unused;
					unused = pool.CreateArray<uint8_t>(size_unused);
					rs.Read(unused.Data(), size_unused);
					rs >> level;
					rs >> index;
					rs >> size_remap;
					remap = pool.CreateArray<uint16_t>(size_remap);
					rs.Read(remap.Data(), sizeof(uint16_t) * size_remap);
					uint8_t flag;
					rs >> flag;

					if (flag)
					{
						reduce = pool.Allocate<PRTable>(1);
						reduce->Load(rs, pool);
					}
					else
						reduce = nullptr;

					falloff_near_size = 200.0f;
					falloff_far_size = 30.0f;
					falloff_power = 2.0f;
				}
				else if (buffer == 2)
				{
					rs >> size_unused;
					unused = pool.CreateArray<uint8_t>(size_unused);
					rs.Read(unused.Data(), size_unused);
					rs >> level;
					rs >> index;
					rs >> size_remap;
					remap = pool.CreateArray<uint16_t>(size_remap);
					rs.Read(remap.Data(), sizeof(uint16_t) * size_remap);
					uint8_t flag;
					rs >> flag;

					if (flag)
					{
						reduce = pool.Allocate<PRTable>(1);
						reduce->Load(rs, pool);
					}
					else
						reduce = nullptr;

					rs >> falloff_near_size;
					rs >> falloff_far_size;
					rs >> falloff_power;
				}
			}
		};
		PRRemap *remap;

		bool Load(ReadStream &rs, MemoryPool &pool);
	};
	Array<Model> model;
	uint32_t currModel;
	Array<uint16_t> flags;
	bool hasShadow;

	struct BODYLOC
	{
		int id;
		int bone_no;
		Vector3 pos;
		Vector3 rot;
		int axis;
	};
	Array<BODYLOC> bodyloc;

	struct ANIM_SET
	{
		int prj_id;
		int spec_id;
	};
	ANIM_SET bindPose;
	ANIM_SET initAnim;
	float velocityRatio;
	int32_t stateRootProject;
	Array<uint32_t> bone_rel;

	bool LoadModel(ReadStream &rs, MemoryPool &pool);
	bool LoadBodyLoc(ReadStream &rs, uint32_t versionBodyLoc);

//public:
	bool Load(ReadStream &rs, MemoryPool &pool);
};
