/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "SpotEffect.hh"
#include "Mesh.hh"

static void sub_4DE2F0(ReadStream &rs)
{
	uint32_t num;
	rs >> num;
	rs.AdvanceBy(num * 5);
	rs.AdvanceBy(1);
}

static void sub_4D7C10(ReadStream &rs)
{
	uint32_t num;
	rs >> num;
	rs.AdvanceBy(num * 8);
	rs.AdvanceBy(1);
}

//T is usually a float, but is also used as a uint8_t
template <typename T = float>
struct VALUE_CONTROL
{
	struct VALUE_INSTANCE
	{
		T value;
		float time;
	};
	Array<VALUE_INSTANCE> values;
	uint8_t flag_random;

	void Load(ReadStream &rs, MemoryPool &pool)
	{
		uint32_t count;
		rs >> count;
		assert(count <= 256); //sanity check

		values = pool.CreateArray<VALUE_INSTANCE>(count);
		for (uint32_t i = 0; i < count; i++)
		{
			rs >> values[i].value;
			rs >> values[i].time;
		}

		rs >> flag_random;
	}
};

struct STREAMER_EFFECT_DATA
{
	uint32_t index_surface_property;
	uint16_t count_segments;
	float wrap_u;
	float wrap_v;
	bool use_surface_material_list_to_animate;
	bool blend_additive;
	bool face_camera;
	bool double_sided;
	VALUE_CONTROL<> radius_modulate;
	VALUE_CONTROL<> edge_wibble_frequency;
	VALUE_CONTROL<> edge_wibble_amplitude;
	VALUE_CONTROL<> point_wibble_frequency;
	VALUE_CONTROL<> point_wibble_amplitude;
	VALUE_CONTROL<> radius;
	VALUE_CONTROL<> offset_u;
	VALUE_CONTROL<uint8_t> colour_a_start;
	VALUE_CONTROL<uint8_t> colour_r_start;
	VALUE_CONTROL<uint8_t> colour_g_start;
	VALUE_CONTROL<uint8_t> colour_b_start;
	VALUE_CONTROL<uint8_t> colour_a_end;
	VALUE_CONTROL<uint8_t> colour_r_end;
	VALUE_CONTROL<uint8_t> colour_g_end;
	VALUE_CONTROL<uint8_t> colour_b_end;
	float initial_length;
	Mesh *pstreamer_mesh;

	void Load(ReadStream &rs, MemoryPool &pool)
	{
		rs >> index_surface_property;
		rs >> count_segments;
		rs >> wrap_u;
		rs >> wrap_v;
		rs >> use_surface_material_list_to_animate;
		rs >> blend_additive;
		rs >> face_camera;
		rs >> double_sided;

		radius_modulate.Load(rs, pool);
		edge_wibble_frequency.Load(rs, pool);
		edge_wibble_amplitude.Load(rs, pool);
		point_wibble_frequency.Load(rs, pool);
		point_wibble_amplitude.Load(rs, pool);
		radius.Load(rs, pool);
		offset_u.Load(rs, pool);

		colour_a_start.Load(rs, pool);
		colour_r_start.Load(rs, pool);
		colour_g_start.Load(rs, pool);
		colour_b_start.Load(rs, pool);
		colour_a_end.Load(rs, pool);
		colour_r_end.Load(rs, pool);
		colour_g_end.Load(rs, pool);
		colour_b_end.Load(rs, pool);

		rs >> initial_length;

//		MakeMesh();
	}
};

void SpotEffect::Load(ReadStream &rs, MemoryPool &pool)
{
	LoadDrawableName(rs, name);

	rs >> type;
	switch (type)
	{
	case SpotEffect::Type::SHOCK_SPHERE:
	{
		float life_max;
		rs >> life_max;
		uint32_t index_surface_property;
		rs >> index_surface_property;
		bool use_surface_material_list_to_animate;
		rs >> use_surface_material_list_to_animate;
		bool blend_additive;
		rs >> blend_additive;

		VALUE_CONTROL size;
		size.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_a;
		colour_a.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_r;
		colour_r.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_g;
		colour_g.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_b;
		colour_b.Load(rs, pool);

		bool looping;
		rs >> looping;

		uint32_t life_loop;
		rs >> life_loop;

//		MakeMesh();
		break;
	}
	case SpotEffect::Type::LAYERED_BILLBOARD:
	{
		//TODO: migrate to VALUE_CONTROL
		rs.AdvanceBy(4);
		rs.AdvanceBy(4);
		rs.AdvanceBy(4);
		rs.AdvanceBy(1);
		rs.AdvanceBy(1);
		rs.AdvanceBy(1);
		rs.AdvanceBy(1);
		rs.AdvanceBy(1);

		uint32_t numA;
		rs >> numA;
		rs.AdvanceBy(numA * 8);
		rs.AdvanceBy(1);

		uint32_t numB;
		rs >> numB;
		rs.AdvanceBy(numB * 8);
		rs.AdvanceBy(1);

		uint32_t numC;
		rs >> numC;
		rs.AdvanceBy(numC * 8);
		rs.AdvanceBy(1);

		uint32_t numD;
		rs >> numD;
		rs.AdvanceBy(numD * 5);
		rs.AdvanceBy(1);

		uint32_t numE;
		rs >> numE;
		rs.AdvanceBy(numE * 5);
		rs.AdvanceBy(1);

		uint32_t numF;
		rs >> numF;
		rs.AdvanceBy(numF * 5);
		rs.AdvanceBy(1);

		sub_4DE2F0(rs);
		sub_4D7C10(rs);

		rs.AdvanceBy(1);
		rs.AdvanceBy(4);
		break;
	}
	case SpotEffect::Type::MESH:
	{
		float life_max;
		rs >> life_max;
		uint32_t drawable_number;
		rs >> drawable_number;
		bool smth0;
		rs >> smth0;

		VALUE_CONTROL size;
		size.Load(rs, pool);

		VALUE_CONTROL<uint8_t> colour_a;
		colour_a.Load(rs, pool);

		VALUE_CONTROL<uint8_t> colour_r;
		colour_r.Load(rs, pool);

		VALUE_CONTROL<uint8_t> colour_g;
		colour_g.Load(rs, pool);

		VALUE_CONTROL<uint8_t> colour_b;
		colour_b.Load(rs, pool);

		bool looping;
		rs >> looping;

		uint32_t lifeLoopStart;
		rs >> lifeLoopStart;
		uint32_t lifeLoopEnd;
		rs >> lifeLoopEnd;

		uint32_t indexSurfaceProperty;
		rs >> indexSurfaceProperty;

		break;
	}
	case SpotEffect::Type::MULTI: //EXPLOSION
	{
		uint16_t count;
		rs >> count;
		rs.AdvanceBy(count * 4);
		break;
	}
	case SpotEffect::Type::EMITTER:
	{
		uint32_t a, b;
		rs >> a;
		rs >> b;
		bool c;
		rs >> c;
		uint32_t d;
		rs >> d;
		break;
	}
	case SpotEffect::Type::LIGHT:
	{
		//TODO: migrate to VALUE_CONTROL
		rs.AdvanceBy(4);
		rs.AdvanceBy(1);
		rs.AdvanceBy(4);
		uint32_t numA;
		rs >> numA;
		rs.AdvanceBy(numA * 8);
		rs.AdvanceBy(1);
		uint32_t numB;
		rs >> numB;
		rs.AdvanceBy(numB * 8);
		rs.AdvanceBy(1);
		uint32_t numC;
		rs >> numC;
		rs.AdvanceBy(numC * 5);
		rs.AdvanceBy(1);
		uint32_t numD;
		rs >> numD;
		rs.AdvanceBy(numD * 5);
		rs.AdvanceBy(1);
		uint32_t numE;
		rs >> numE;
		rs.AdvanceBy(numE * 5);

		rs.AdvanceBy(7);

		rs.AdvanceBy(3);
		uint32_t numF;
		rs >> numF;
		rs.AdvanceBy(numF * 5);

		rs.AdvanceBy(1);
		uint32_t numG;
		rs >> numG;
		rs.AdvanceBy(numG * 8);

		rs.AdvanceBy(1);

		break;
	}
	case SpotEffect::Type::HEMI_SPHERE:
	{
		float life_max;
		rs >> life_max;
		uint32_t index_surface_property;
		rs >> index_surface_property;
		bool use_surface_material_list_to_animate;
		rs >> use_surface_material_list_to_animate;
		bool blend_additive;
		rs >> blend_additive;
		float angle_start;
		rs >> angle_start;
		float angle_end;
		rs >> angle_end;
		float wrap_u;
		rs >> wrap_u;
		float wrap_v;
		rs >> wrap_v;
		uint16_t count_rings;
		rs >> count_rings;
		uint16_t count_segments;
		rs >> count_segments;
		bool double_sided;
		rs >> double_sided;
		bool facing;
		rs >> facing;
		bool looping;
		rs >> looping;

		VALUE_CONTROL size;
		size.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_a;
		colour_a.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_r;
		colour_r.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_g;
		colour_g.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_b;
		colour_b.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_angle_a;
		colour_angle_a.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_angle_r;
		colour_angle_r.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_angle_g;
		colour_angle_g.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_angle_b;
		colour_angle_b.Load(rs, pool);

		uint32_t lifeLoop;
		rs >> lifeLoop;

//		MakeMesh();
		break;
	}
	case SpotEffect::Type::SOUND:
	{
		uint32_t a;
		rs >> a;
		bool b;
		rs >> b;
		uint32_t c;
		rs >> c;
		uint16_t d;
		rs >> d;
		bool e;
		rs >> e;
		break;
	}
	case SpotEffect::Type::TOGGLE_LIGHTMAP:
	{
		uint32_t thing;
		rs >> thing;
		break;
	}
	case SpotEffect::Type::STREAMER:
	{
		float lifeMax;
		rs >> lifeMax;
		bool looping;
		rs >> looping;
		bool useMatchingArray;
		rs >> useMatchingArray;
		VALUE_CONTROL arrayWibbleAmplitude;
		arrayWibbleAmplitude.Load(rs, pool);
		VALUE_CONTROL arrayWibbleFrequency;
		arrayWibbleFrequency.Load(rs, pool);
		STREAMER_EFFECT_DATA data;
		data.Load(rs, pool);
		uint32_t lifeLoop;
		rs >> lifeLoop;
		break;
	}
	case SpotEffect::Type::TEXTURE_HOLDER:
	{
		uint32_t count;
		rs >> count;
		rs.AdvanceBy(count * 4);
		break;
	}
	case SpotEffect::Type::SPAWNER:
	{
		rs.AdvanceBy(4);
		rs.AdvanceBy(4);
		rs.AdvanceBy(4);
		rs.AdvanceBy(4);
		rs.AdvanceBy(1);
		break;
	}
	case SpotEffect::Type::CAMERA_SHAKE:
	{
		float life_max;
		rs >> life_max;
		uint8_t looping;
		rs >> looping;
		uint8_t scale_with_distance;
		rs >> scale_with_distance;
		uint8_t verticle_rectify;
		rs >> verticle_rectify;

		VALUE_CONTROL radius;
		radius.Load(rs, pool);
		VALUE_CONTROL center_strength;
		center_strength.Load(rs, pool);
		VALUE_CONTROL center_random;
		center_random.Load(rs, pool);
		VALUE_CONTROL verticle_amplitude;
		verticle_amplitude.Load(rs, pool);
		VALUE_CONTROL verticle_frequency;
		verticle_frequency.Load(rs, pool);
		VALUE_CONTROL tilt_angle;
		tilt_angle.Load(rs, pool);

		uint32_t life_loop;
		rs >> life_loop;
		break;
	}
	case SpotEffect::Type::CUTSCENE:
	{
		rs >> cutScene;
		break;
	}
	case SpotEffect::Type::DISC:
	{
		float life_max;
		rs >> life_max;
		uint32_t index_surface_property;
		rs >> index_surface_property;
		bool use_surface_material_list_to_animate;
		rs >> use_surface_material_list_to_animate;
		bool blend_additive;
		rs >> blend_additive;
		float wrap_u;
		rs >> wrap_u;
		float wrap_v;
		rs >> wrap_v;
		uint16_t count_rings;
		rs >> count_rings;
		uint16_t count_segments;
		rs >> count_segments;
		bool double_sided;
		rs >> double_sided;
		bool facing;
		rs >> facing;
		bool looping;
		rs >> looping;

		VALUE_CONTROL radius_inner;
		radius_inner.Load(rs, pool);
		VALUE_CONTROL radius_outter;
		radius_outter.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_a;
		colour_a.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_r;
		colour_r.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_g;
		colour_g.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_b;
		colour_b.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_radius_a;
		colour_radius_a.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_radius_r;
		colour_radius_r.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_radius_g;
		colour_radius_g.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_radius_b;
		colour_radius_b.Load(rs, pool);

		uint32_t lifeLoop;
		rs >> lifeLoop;
//		MakeMesh();
		break;
	}
	case SpotEffect::Type::CYLINDER:
	{
		float life_max;
		rs >> life_max;
		uint32_t index_surface_property;
		rs >> index_surface_property;
		bool use_surface_material_list_to_animate;
		rs >> use_surface_material_list_to_animate;
		bool blend_additive;
		rs >> blend_additive;
		float wrap_u;
		rs >> wrap_u;
		float wrap_v;
		rs >> wrap_v;
		uint16_t count_rings;
		rs >> count_rings;
		uint16_t count_segments;
		rs >> count_segments;
		bool double_sided;
		rs >> double_sided;
		bool facing;
		rs >> facing;
		bool looping;
		rs >> looping;

		VALUE_CONTROL size;
		size.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_a;
		colour_a.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_r;
		colour_r.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_g;
		colour_g.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_b;
		colour_b.Load(rs, pool);
		VALUE_CONTROL radius_delta;
		radius_delta.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_delta_a;
		colour_delta_a.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_delta_r;
		colour_delta_r.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_delta_g;
		colour_delta_g.Load(rs, pool);
		VALUE_CONTROL<uint8_t> colour_delta_b;
		colour_delta_b.Load(rs, pool);

		uint32_t lifeLoop;
		rs >> lifeLoop;

		VALUE_CONTROL length;
		length.Load(rs, pool);

//		MakeMesh();
		break;
	}
	case SpotEffect::Type::TRAILER:
	{
		float life_max;
		rs >> life_max;
		STREAMER_EFFECT_DATA data;
		data.Load(rs, pool);
		break;
	}
	case SpotEffect::Type::LIGHT_VOLUME:
	{
		assert(0);
		break;
	}
	case SpotEffect::Type::DAMAGE_SPHERE:
	{
		uint32_t a, b, c;
		rs >> a;
		rs >> b;
		rs >> c;
		break;
	}
	default:
		break;
	}
}
