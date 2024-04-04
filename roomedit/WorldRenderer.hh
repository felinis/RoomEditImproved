#pragma once
#include "shaders/ColoredSurfaceGraphicsPipeline.hh"
#include "shaders/LightMappedSurfaceGraphicsPipeline.hh"
#include "shaders/PhongSurfaceGraphicsPipeline.hh"
#include "Document.hh"
#include "BBox.hh"

////////////////////////////////////////////////////////////////////////////////////////////////

//TODO: rename to BakedColoredVertex
struct ColoredVertex
{
	float position[3];
	float color[3];
};

//the color is sent dynamically
struct DynColoredVertex
{
	float position[3];
};

struct LightMappedVertex
{
	float position[3];
	float texcoordDiffuse[2];
	float texcoordLightmap[2];
};

struct PhongVertex
{
	float position[3];
	float normal[3];
	float texcoordDiffuse[2];
};

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

struct ObjectMeshPart
{
	uint32_t numIndices;
	int32_t indexSurfaceProperty;
};

struct ObjectMesh
{
	sbMesh mesh;
	Array<ObjectMeshPart> parts;
};

////////////////////////////////////////////////////////////////////////////////////////////////

class LineRenderer
{
	struct Line
	{
		ColoredVertex verts[2];

		Line() :
			verts()
		{
		}

		Line(const Vector &start, const Vector &end, const Vector &color)
		{
			verts[0].position[0] = start.x;
			verts[0].position[1] = start.y;
			verts[0].position[2] = start.z;
			verts[0].color[0] = color.x;
			verts[0].color[1] = color.y;
			verts[0].color[2] = color.z;
			verts[1].position[0] = end.x;
			verts[1].position[1] = end.y;
			verts[1].position[2] = end.z;
			verts[1].color[0] = color.x;
			verts[1].color[1] = color.y;
			verts[1].color[2] = color.z;
		}
	};
	FixedArray<Line, 1024> lines;

public:
	constexpr LineRenderer() = default;

	void AddLine(const Vector &start, const Vector &end, const Vector &color = Vector(1.0f, 1.0f, 1.0f))
	{
		Line line(start, end, color);
		lines.Add(line);
	}

	void AddBox(const BBox &box, const Vector &color = Vector(0.5f, 0.5f, 0.0f))
	{
		//define the 8 vertices of the bounding box
		Vector vertices[8] = {
			box.min,
			Vector(box.min.x, box.min.y, box.max.z),
			Vector(box.min.x, box.max.y, box.min.z),
			Vector(box.min.x, box.max.y, box.max.z),
			Vector(box.max.x, box.min.y, box.min.z),
			Vector(box.max.x, box.min.y, box.max.z),
			Vector(box.max.x, box.max.y, box.min.z),
			box.max
		};

		//draw lines between the vertices to form the bounding box
		AddLine(vertices[0], vertices[1], color);
		AddLine(vertices[0], vertices[2], color);
		AddLine(vertices[0], vertices[4], color);
		AddLine(vertices[1], vertices[3], color);
		AddLine(vertices[1], vertices[5], color);
		AddLine(vertices[2], vertices[3], color);
		AddLine(vertices[2], vertices[6], color);
		AddLine(vertices[3], vertices[7], color);
		AddLine(vertices[4], vertices[5], color);
		AddLine(vertices[4], vertices[6], color);
		AddLine(vertices[5], vertices[7], color);
		AddLine(vertices[6], vertices[7], color);
	}

	void Flush()
	{
		lines.Flush();
	}

	void Render(const sbRenderer &renderer)
	{
		//bail out if nothing to draw
		if (lines.Count() == 0)
			return;

		//we can draw directly by providing the raw lines data, very handy!
		renderer.DrawDynamic(lines.Data(), lines.Count() * 2, sizeof(ColoredVertex), 0, 0);

		Flush();
	}
#if 0
	void Render(const Document &doc, const sbRenderer &renderer)
	{
		Flush();

		//draw all selected rooms' bounding boxes
		for (uint32_t i = 0; i < doc.world.rooms.Count(); i++)
		{
			if (!doc.IsSelected(i))
				continue;

			const auto &room = doc.world.rooms[i];
			BBox box = ComputeRoomBBox(room);
			AddBox(box);
		}

		Render(renderer);
	}
#endif
};

////////////////////////////////////////////////////////////////////////////////////////////////

inline BBox ComputeRoomBBox(const Room &room)
{
	return BBox(
		Vector(room.mesh.minExtent.x, room.mesh.minExtent.y, room.mesh.minExtent.z) * room.scale + Vector(room.position.x, room.position.y, room.position.z),
		Vector(room.mesh.maxExtent.x, room.mesh.maxExtent.y, room.mesh.maxExtent.z) * room.scale + Vector(room.position.x, room.position.y, room.position.z)
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////

class WorldRenderer
{
	friend class RoomRenderer; //we need to be able to access WorldRenderer's private contents from RoomRenderer
	friend class ObjectRenderer; //same

	sbRenderer &renderer;

	static constexpr uint32_t FALLBACK_TEXTURE_INDEX = 0;
	GPUResource fallbackTexture; //the texture that is used in case one does not exist

	LineRenderer lineRenderer;

	ColoredSurfaceGraphicsPipeline csgPipeline;
	LightMappedSurfaceGraphicsPipeline lmsgPipeline;
	PhongSurfaceGraphicsPipeline psgPipeline;

	MemoryPool pool;

	//device-specific buffers
	Array<RoomMesh> roomMeshes;
	Array<ObjectMesh> meshes;
	Array<GPUResource> textures;
	Array<ObjectMesh> actorMeshes;

public:
	//TODO: put this in the Lightmapping pipeline
	void UseDiffuseAndLightmap(uint32_t texindexDiffuse, uint32_t texindexLightmap);
	void UseFullbrightDiffuse(uint32_t texindexDiffuse);
	void UseFallbackDiffuseAndLightmap();

	void SetWorldViewProjectionMatrix(const Matrix &m);

	//TODO: put this in the Phong pipeline
	void UsePhongFallbackDiffuse();
	void UsePhongDiffuse(uint32_t texindexDiffuse);

	WorldRenderer(sbRenderer &renderer):
		renderer(renderer),
		fallbackTexture()
	{}

	bool Create();
	void Destroy();

	bool LoadWorld(const GameWorld &world);
	void UnloadWorld();

	void Render(const Document &document, const Matrix &viewProjection);
};
