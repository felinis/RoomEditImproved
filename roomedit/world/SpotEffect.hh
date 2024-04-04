#pragma once
#include "common.hh"
#include "sbfilesystem/ReadStream.hh"
#include "sbmemory/MemoryPool.hh"

struct SpotEffect
{
	char name[MAX_DRAWABLE_NAME_LENGTH];

	enum class Type : uint16_t
	{
		START,
		NONE = 0,
		SHOCK_SPHERE,
		LAYERED_BILLBOARD,
		MESH,
		MULTI,
		EMITTER,
		LIGHT,
		HEMI_SPHERE,
		SOUND,
		TOGGLE_LIGHTMAP,
		STREAMER,
		TEXTURE_HOLDER,
		SPAWNER,
		CAMERA_SHAKE,
		CUTSCENE,
		DISC,
		CYLINDER,
		TRAILER,
		LIGHT_VOLUME,
		DAMAGE_SPHERE,
		END
	};
	Type type;

	union
	{
		struct SpotEffectLight
		{

		};
		SpotEffectLight light;

		struct SpotEffectCameraShake
		{
			float life_max;
			float life_loop;
			bool looping;
			bool scale_with_distance;
			bool verticle_rectify;
		};
		SpotEffectCameraShake cameraShake;

		struct SpotEffectCutScene
		{
			int cutsceneId;
		};
		SpotEffectCutScene cutScene;
	};

	void Load(ReadStream &rs, MemoryPool &pool);
};
