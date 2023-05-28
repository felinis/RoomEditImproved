#include "world_renderer.h"
#include <assert.h>

static MemoryPool pool;

//device-specific buffers
static Array<RoomMesh> roomMeshes;
static Array<GPUResource> textures;

bool CreateWorldRenderer(const GameWorld& world, sbRenderer& renderer)
{
	//save the internal state of the renderer here so that whenever
	//we want to free game world resources allocated using the renderer,
	//we can just revert to the saved state
	renderer.SaveInternalState();

	pool.Create(1024 * 1024 * 20); //20MiB is sufficient to load any type of Eden level

	//create a mesh buffer for all the rooms
	roomMeshes = pool.CreateArray<RoomMesh>(world.rooms.Count());
	for (uint32_t i = 0; i < world.rooms.Count(); i++)
	{
		auto &room = world.rooms[i];
		auto &roomMesh = room.mesh;

		//count how many parts/textures this room mesh has
		uint32_t numTextureIndexChanges = 0;
		{
			auto lastSeenTextureIndex = roomMesh.faces[0].indexSurfaceProperty;
			for (auto& face : roomMesh.faces)
			{
				if (face.indexSurfaceProperty != lastSeenTextureIndex)
				{
					numTextureIndexChanges++;
					lastSeenTextureIndex = face.indexSurfaceProperty;
				}
			}
		}
		
		uint32_t numParts = numTextureIndexChanges + 1;

		RoomMesh mesh;
		mesh.parts = pool.CreateArray<RoomPart>(numParts);
		uint32_t l = 0;
		auto lastSeenTextureIndex = roomMesh.faces[0].indexSurfaceProperty;

		//count the number of indices this mesh is using
		uint32_t numIndices = 0;
		uint32_t lastNumIndices = 0;
		for (uint32_t a = 0; a < roomMesh.faces.Count(); a++)
		{
			auto& face = roomMesh.faces[a];

			if (face.indexSurfaceProperty == 3)
				continue;

			//has the texture index changed?
			bool textureIndexChanged = face.indexSurfaceProperty != lastSeenTextureIndex;
			if (textureIndexChanged)
			{
				lastNumIndices = numIndices;
				lastSeenTextureIndex = face.indexSurfaceProperty;
			}

			assert(face.numVerts >= 3);
			switch (face.typePoly)
			{
				/* QUAD (4 vertices, indices [0, 1, 2, 3], 6 index count ) */
			case 4:
				numIndices += face.numVerts;
				break;

				/* TRIANGLE (3 vertices = indices [0, 1, 2], 3 index nums used) */
				/* 6 vertices, indices [0, 1, 2, 3, 4, 5], 12 index nums used */
			case 6:
				numIndices += 3 * face.numVerts - 6;
				break;
			default:
				assert(0);
			}

			bool lastFace = a == (roomMesh.faces.Count() - 1);
			bool nextFaceHasDifferentTextureIndex = (!lastFace && roomMesh.faces[a+1].indexSurfaceProperty != face.indexSurfaceProperty);
			if (lastFace || nextFaceHasDifferentTextureIndex)
			{
				//write how many indices did we have until now
				mesh.parts[l].numIndices = numIndices - lastNumIndices;
				mesh.parts[l].indexSurfaceProperty = face.indexSurfaceProperty;
				mesh.parts[l++].texindexLightmap = face.lightmapIndex;
			}
		}

		auto indices = pool.Allocate<uint32_t>(numIndices);
		uint32_t* currentIndex = indices;

		//form the indices
		for (auto& face : roomMesh.faces)
		{
			if (face.indexSurfaceProperty == 3)
				continue;

			switch (face.typePoly)
			{
			case 4:
				for (uint32_t j = 0; j < face.numVerts; j++)
					*currentIndex++ = face.vertexIndices[j];
				break;
			case 6:
			{
				const unsigned short startingIndex = face.vertexIndices[0];
				const unsigned short *nextIndex = &face.vertexIndices[1];
				for (uint32_t j = 2; j < face.numVerts; j++)
				{
					*currentIndex++ = startingIndex;
					*currentIndex++ = *nextIndex++;
					*currentIndex++ = *nextIndex;
				}
				break;
			}
			default:
				assert(0);
			}
		}

		//create the verts
		uint32_t numCorners = (uint32_t)roomMesh.corners.Count();
		auto* verts = pool.Allocate<Vertex>(numCorners);
		for (uint32_t j = 0; j < numCorners; j++)
		{
			unsigned short order = roomMesh.corners[j].index;

			//fill position
			verts[j].position[0] = roomMesh.positions[order].x;
			verts[j].position[1] = -roomMesh.positions[order].y; //invert Y
			verts[j].position[2] = roomMesh.positions[order].z;

			//apply scale
			verts[j].position[0] *= room.scale;
			verts[j].position[1] *= room.scale;
			verts[j].position[2] *= room.scale;

			//apply transform
			verts[j].position[0] += room.position.x;
			verts[j].position[1] -= room.position.y; //invert Y
			verts[j].position[2] += room.position.z;

			//fill texcoords
			memcpy(&verts[j].texcoordDiffuse, &roomMesh.corners[j].textureUV, 8);
			memcpy(&verts[j].texcoordLightmap, &roomMesh.corners[j].lightmapUV, 8);
		}

		//copy per-face normals if requested
#if 0
		if (roomMesh.flags & 2)
		{
			for (auto &face : roomMesh.faces)
			{
				for (auto &vertexIndex : face.vertexIndices)
				{
					memcpy(verts[vertexIndex].color, &face.normal, sizeof(Vector3));
					verts[vertexIndex].color[3] = 1.0f;
				}
			}
		}
#endif

		mesh.mesh = renderer.CreateMesh(verts, numCorners, indices, numIndices);
		roomMeshes[i] = std::move(mesh);
	}

	//create a texture buffer for all the textures
	textures = pool.CreateArray<GPUResource>(world.textures.Count());
	auto poolSafeResetPoint = pool.GetOffset(); //get the checkpoint just after creating the texture array
	for (uint32_t i = 0; i < world.textures.Count(); i++)
	{
		const auto &texture = world.textures[i];
		if (!texture.frames.Count())
			continue;

		const auto &frame = texture.frames[0];
		DXGI_FORMAT format;
		switch (frame.magic)
		{
		case '1TXD':
			format = DXGI_FORMAT_BC1_UNORM;
			break;
		case '3TXD':
			format = DXGI_FORMAT_BC2_UNORM;
			break;
		case '5TXD':
			format = DXGI_FORMAT_BC3_UNORM;
			break;
		default:
			assert(0);
		}

		textures[i] = renderer.CreateTexture(frame.data, frame.size, frame.width, frame.height, format, i);

		pool.FlushFrom(poolSafeResetPoint); //we can safely start overwriting memory from here
	}

	return true;
}

void DestroyWorldRenderer(sbRenderer& renderer)
{
	for (auto &texture : textures)
		renderer.DestroyTexture(texture);

	for (auto &roomMesh : roomMeshes)
		renderer.DestroyMesh(roomMesh.mesh);

	pool.Destroy();

	//just restore the previously saved renderer state, simple as that!
	renderer.RestoreInternalState();
}

void RenderWorld(const GameWorld& world, sbRenderer& renderer)
{
	for (auto &roomMesh : roomMeshes)
	{
		//bind the room mesh
		renderer.BindMesh(roomMesh.mesh);

		//for every part of the mesh...
		uint32_t startIndex = 0;
		for (const auto &part : roomMesh.parts)
		{
			int textureIndex = world.GetBaseTextureIndex(part.indexSurfaceProperty);
			int lightmapIndex = part.texindexLightmap - GameWorld::NUM_SYSTEM_TEXTURES;

			//bind its textures...
			bool noDiffuse = textureIndex < 0;
			bool noLightmap = lightmapIndex < 0;
			if (noDiffuse)
				renderer.UseFallbackDiffuseAndLightmap();
			else if (noLightmap)
				renderer.UseFullbrightDiffuse(textureIndex);
			else
				renderer.UseDiffuseAndLightmap(textureIndex, lightmapIndex);

			//draw the adequate number of indices
			renderer.DrawBoundMesh(part.numIndices, startIndex);
			startIndex += part.numIndices;
		}
	}
}
