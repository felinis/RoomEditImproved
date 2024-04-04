/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "WorldRenderer.hh"
#include <assert.h>

//for LIGHTMAPPED
void WorldRenderer::UseDiffuseAndLightmap(uint32_t texindexDiffuse, uint32_t texindexLightmap)
{
	renderer.UseTwoTextures(texindexDiffuse, texindexLightmap);
}

//for LIGHTMAPPED
void WorldRenderer::UseFullbrightDiffuse(uint32_t texindexDiffuse)
{
	renderer.UseTwoTextures(texindexDiffuse, FALLBACK_TEXTURE_INDEX);
}

//for LIGHTMAPPED
void WorldRenderer::UseFallbackDiffuseAndLightmap()
{
	renderer.UseTwoTextures(FALLBACK_TEXTURE_INDEX, FALLBACK_TEXTURE_INDEX);
}

void WorldRenderer::SetWorldViewProjectionMatrix(const Matrix &m)
{
	renderer.SetWorldViewProjectionMatrix(m);
}

//for PHONG
void WorldRenderer::UsePhongFallbackDiffuse()
{
	renderer.UseOneTexture(FALLBACK_TEXTURE_INDEX);
}

//for PHONG
void WorldRenderer::UsePhongDiffuse(uint32_t texindexDiffuse)
{
	renderer.UseOneTexture(texindexDiffuse);
}

bool WorldRenderer::Create()
{
	//create the PSOs
	{
		//create light-mapped pipeline
		if (!renderer.CreatePipeline(lmsgPipeline))
			return false;

		//create phong pipeline
		//we create a second pipeline that is similar to the lightmapped,
		//so we pass a cached state blob to make things faster
		if (!renderer.CreatePipeline(psgPipeline)) //TODO: add cached state
			return false;

		//create colored pipeline
		if (!renderer.CreatePipeline(csgPipeline)) //TODO: add cached state
			return false;
	}

	//create the fallback texture
	{
		struct RGBAColor
		{
			uint8_t r, g, b, a;
		};
		RGBAColor data[1] =
		{
			255, 255, 255, 255
		};
		fallbackTexture = renderer.CreateTexture(data, sizeof(data), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, FALLBACK_TEXTURE_INDEX);
	}

	//create the light mesh
//	if (!lightBulbMesh.Create(renderer))
//		return false;

	return true;
}

void WorldRenderer::Destroy()
{
//	lightBulbMesh.Destroy(renderer);

	renderer.DestroyTexture(fallbackTexture);

	renderer.DestroyPipeline(csgPipeline);
	renderer.DestroyPipeline(psgPipeline);
	renderer.DestroyPipeline(lmsgPipeline);
}

bool WorldRenderer::LoadWorld(const GameWorld &world)
{
	//save the internal state of the renderer here so that whenever
	//we want to free game world resources allocated using the renderer,
	//we can just revert to that saved state
	renderer.SaveInternalState();

	pool.Create(1024 * 1024 * 20); //20MiB is sufficient to load any type of Eden level

	//create a mesh buffer for all the rooms
	roomMeshes = pool.CreateArray<RoomMesh>(world.rooms.Count());
	for (uint32_t i = 0; i < world.rooms.Count(); i++)
	{
		auto &room = world.rooms[i];
		auto &roomMesh = room.mesh;

		//if this room contains 0 vertices, it can not be drawn so we skip it
		if (roomMesh.numVerts == 0)
			continue;

		//count how many parts/textures this room mesh has
		uint32_t numTextureIndexChanges = 0;
		{
			auto lastSeenTextureIndex = roomMesh.faces[0].indexSurfaceProperty;
			for (auto &face : roomMesh.faces)
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
			auto &face = roomMesh.faces[a];

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
			bool nextFaceHasDifferentTextureIndex = (!lastFace && roomMesh.faces[a + 1].indexSurfaceProperty != face.indexSurfaceProperty);
			if (lastFace || nextFaceHasDifferentTextureIndex)
			{
				//write how many indices did we have until now
				mesh.parts[l].numIndices = numIndices - lastNumIndices;
				mesh.parts[l].indexSurfaceProperty = face.indexSurfaceProperty;
				mesh.parts[l].texindexLightmap = face.lightmapIndex;
				l++;
			}
		}

		auto indices = pool.Allocate<uint32_t>(numIndices);
		uint32_t *currentIndex = indices;

		//form the indices
		for (auto &face : roomMesh.faces)
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
		auto *verts = pool.Allocate<LightMappedVertex>(numCorners);
		for (uint32_t j = 0; j < numCorners; j++)
		{
			unsigned short order = roomMesh.corners[j].index;

			//fill position
			verts[j].position[0] = roomMesh.positions[order].x;
			verts[j].position[1] = roomMesh.positions[order].y;
			verts[j].position[2] = roomMesh.positions[order].z;

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

	//create a mesh buffer for all the meshes
	meshes = pool.CreateArray<ObjectMesh>(world.meshes.Count());
	for (uint32_t i = 0; i < world.meshes.Count(); i++)
	{
		auto &wmesh = world.meshes[i];

		//if this room contains 0 vertices, it can not be drawn so we skip it
		if (wmesh.numVerts == 0)
			continue;

		//count how many parts/textures this room mesh has
		uint32_t numTextureIndexChanges = 0;
		{
			auto lastSeenTextureIndex = wmesh.faces[0].indexSurfaceProperty;
			for (auto &face : wmesh.faces)
			{
				if (face.indexSurfaceProperty != lastSeenTextureIndex)
				{
					numTextureIndexChanges++;
					lastSeenTextureIndex = face.indexSurfaceProperty;
				}
			}
		}

		uint32_t numParts = numTextureIndexChanges + 1;

		ObjectMesh mesh;
		mesh.parts = pool.CreateArray<ObjectMeshPart>(numParts);
		uint32_t l = 0;
		auto lastSeenTextureIndex = wmesh.faces[0].indexSurfaceProperty;

		//count the number of indices this mesh is using
		uint32_t numIndices = 0;
		uint32_t lastNumIndices = 0;
		for (uint32_t a = 0; a < wmesh.faces.Count(); a++)
		{
			auto &face = wmesh.faces[a];

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

			bool lastFace = a == (wmesh.faces.Count() - 1);
			bool nextFaceHasDifferentTextureIndex = (!lastFace && wmesh.faces[a + 1].indexSurfaceProperty != face.indexSurfaceProperty);
			if (lastFace || nextFaceHasDifferentTextureIndex)
			{
				//write how many indices did we have until now
				mesh.parts[l].numIndices = numIndices - lastNumIndices;
				mesh.parts[l].indexSurfaceProperty = face.indexSurfaceProperty;
				l++;
			}
		}

		auto indices = pool.Allocate<uint32_t>(numIndices);
		uint32_t *currentIndex = indices;

		//form the indices
		for (auto &face : wmesh.faces)
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
		uint32_t numCorners = (uint32_t)wmesh.corners.Count();
		auto *verts = pool.Allocate<PhongVertex>(numCorners);
		for (uint32_t j = 0; j < numCorners; j++)
		{
			unsigned short order = wmesh.corners[j].index;

			//fill position
			verts[j].position[0] = wmesh.positions[order].x;
			verts[j].position[1] = wmesh.positions[order].y;
			verts[j].position[2] = wmesh.positions[order].z;

			//fill smooth normal
			verts[j].normal[0] = wmesh.normals[order].x;
			verts[j].normal[1] = wmesh.normals[order].y;
			verts[j].normal[2] = wmesh.normals[order].z;

			//fill texcoords
			memcpy(&verts[j].texcoordDiffuse, &wmesh.corners[j].textureUV, 8);
		}

		//copy per-face normals if requested
#if 0
		if (wmesh.flags & 2)
		{
			for (auto &face : wmesh.faces)
			{
				for (auto &vertexIndex : face.vertexIndices)
				{
					memcpy(verts[vertexIndex].normal, &face.normal, sizeof(Vector3));
				}
			}
		}
#endif

		mesh.mesh = renderer.CreateMesh(verts, numCorners, indices, numIndices);
		meshes[i] = std::move(mesh);
	}

	//create a texture buffer for all the textures
	textures = pool.CreateArray<GPUResource>(world.textures.Count());
	for (uint32_t i = 0; i < world.textures.Count(); i++)
	{
		const auto &texture = world.textures[i];
		if (!texture.HasFrames())
			continue; //no texture to create here

		const auto &frame = texture.GetFrame(0);
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
	}

	return true;
}

void WorldRenderer::UnloadWorld()
{
	for (auto &mesh : actorMeshes)
		renderer.DestroyMesh(mesh.mesh);

	for (auto &texture : textures)
		renderer.DestroyTexture(texture);

	for (auto &mesh : meshes)
		renderer.DestroyMesh(mesh.mesh);

	for (auto &roomMesh : roomMeshes)
		renderer.DestroyMesh(roomMesh.mesh);

	pool.Destroy();

	//just restore the previously saved renderer state, simple as that!
	renderer.RestoreInternalState();
}

static void TranslateRelative(Matrix &matrix, float x, float y, float z)
{
	matrix[3].x += x * matrix[0].x + y * matrix[1].x + z * matrix[2].x;
	matrix[3].y += x * matrix[0].y + y * matrix[1].y + z * matrix[2].y;
	matrix[3].z += x * matrix[0].z + y * matrix[1].z + z * matrix[2].z;
}

static void RotateXYZ(Matrix &matrix, float rx, float ry, float rz)
{
	float sinValue;
	float cosValue;
	Vector temp;

	if (rz != 0.0f)
	{
		sinValue = sinf(rz);
		cosValue = cosf(rz);

		temp = matrix[0];
		matrix[0] = temp * cosValue + matrix[1] * sinValue;
		matrix[1] = matrix[1] * cosValue - temp * sinValue;
	}
	if (rx != 0.0f)
	{
		sinValue = sinf(rx);
		cosValue = cosf(rx);

		temp = matrix[1];
		matrix[1] = temp * cosValue + matrix[2] * sinValue;
		matrix[2] = matrix[2] * cosValue - temp * sinValue;
	}
	if (ry != 0.0f)
	{
		sinValue = sinf(ry);
		cosValue = cosf(ry);

		temp = matrix[2];
		matrix[2] = temp * cosValue + matrix[0] * sinValue;
		matrix[0] = matrix[0] * cosValue - temp * sinValue;
	}
}

static void Scale(Matrix &matrix, float scale)
{
	matrix[0] *= scale;
	matrix[1] *= scale;
	matrix[2] *= scale;
}

#if 0
class HierarchyRenderer
{
	sbRenderer &renderer;
	const Matrix &viewProjection;

	void SetSpecificPipeline(const GameWorld &world, const ObjectMeshPart &part)
	{
		int textureIndex = world.GetBaseTextureIndex(part.indexSurfaceProperty);
		bool noDiffuse = textureIndex < 0;
		if (noDiffuse)
			renderer.UsePhongFallbackDiffuse();
		else
			renderer.UsePhongDiffuse(textureIndex);
	}

public:
	HierarchyRenderer(sbRenderer &renderer, const Matrix &viewProjection) :
		renderer(renderer), viewProjection(viewProjection)
	{}

	void Render(const GameWorld &world, uint32_t index)
	{
		const auto &actor = world.actorWAD.actors[index];
		const auto &internalMesh = actorMeshes[index];

		//if this actor mesh is empty, we skip it
		if (internalMesh.mesh.numIndices == 0)
			return;

		//bind the actor mesh
		renderer.BindMesh<PhongVertex>(internalMesh.mesh);

		//for every part of the mesh...
		uint32_t startIndex = 0;
		for (const auto &part : internalMesh.parts)
		{
			SetSpecificPipeline(world, part);

			//draw the adequate number of indices
			renderer.DrawBoundMesh(part.numIndices, startIndex);
			startIndex += part.numIndices;
		}
	}
};
#endif

class ObjectRenderer
{
	WorldRenderer &worldRenderer;
	const Matrix &viewProjection;

	void SetSpecificPipeline(const GameWorld &world, const ObjectMeshPart &part)
	{
		auto textureIndex = world.GetBaseTextureIndex(part.indexSurfaceProperty);
		bool noDiffuse = textureIndex < 0;
		if (noDiffuse)
			worldRenderer.UsePhongFallbackDiffuse();
		else
			worldRenderer.UsePhongDiffuse(textureIndex);
	}

	void DrawMesh(const GameWorld &world, Object *object)
	{
		uint32_t index = object->drawableNumber.GetID();
		assert(index < world.meshes.Count());

		const ObjectMesh &mesh = worldRenderer.meshes[index];

		//if this mesh is empty, we skip it
		if (mesh.mesh.numIndices == 0)
			return;

		worldRenderer.renderer.BindMesh<PhongVertex>(mesh.mesh);
//		const Room &parentRoom = world.rooms[object->location];

		//draw every part of the mesh
		uint32_t startIndex = 0;
		for (const auto &part : mesh.parts)
		{
			SetSpecificPipeline(world, part);

			//draw the adequate number of indices
			worldRenderer.renderer.DrawBoundMesh(part.numIndices, startIndex);
			startIndex += part.numIndices;
		}
	}

public:
	ObjectRenderer(WorldRenderer &worldRenderer, const Matrix &viewProjection):
		worldRenderer(worldRenderer), viewProjection(viewProjection)
	{}

	void Render(const GameWorld &world, Object *object, const Matrix &parentTransform)
	{
		Matrix objectTransform = parentTransform; //make a copy
//		Vector objectPosition = Vector(object->position.x, object->position.y, object->position.z);
//		objectTransform.SetTranslation(parentTransform.GetTranslation() + objectPosition);
//		objectTransform.SetScale(object->scale);
		TranslateRelative(objectTransform, object->position.x, object->position.y, object->position.z);
		RotateXYZ(objectTransform, object->rotation.x, object->rotation.y, object->rotation.z);
		Scale(objectTransform, object->scale);

		worldRenderer.renderer.SetWorldViewProjectionMatrix(viewProjection * objectTransform);

		MESH_TYPE meshType = object->drawableNumber.GetMeshType();
//		uint32_t index = object->drawableNumber.GetID();
		switch (meshType)
		{
		case MT_MESH:
		{
			DrawMesh(world, object);
			break;
		}
//		case MT_EMITTER:
//		{
//			assert(index < world.emitters.Count());
//			break;
//		}
//		case MT_HIERARCHY:
//		{
//			break;
//		}
//		case MT_TRIGGER:
//		{
//			break;
//		}
//		case MT_SPOT_EFFECT:
//		{
//			assert(index < world.spotEffects.Count());
//			break;
//		}
		default:
			return;
		}

		for (Object *subObject = object->objects; subObject; subObject = subObject->next)
			Render(world, subObject, objectTransform);
	}
};

class RoomRenderer
{
	WorldRenderer &worldRenderer; //the RoomRenderer cannot exist without a WorldRenderer
	LineRenderer &lineRenderer;
	const Matrix &viewProjection;
	const bool &drawLighting;

	void SetSpecificPipeline(const GameWorld &world, const RoomPart &part)
	{
		auto textureIndex = world.GetBaseTextureIndex(part.indexSurfaceProperty);
		auto lightmapIndex = part.texindexLightmap - GameWorld::NUM_SYSTEM_TEXTURES;

		//bind its textures...
		bool noDiffuse = textureIndex < 0;
		bool noLightmap = lightmapIndex < 0 || !drawLighting;
		if (noDiffuse || noLightmap)
			worldRenderer.UseFallbackDiffuseAndLightmap();
//		else if (noLightmap)
//			worldRenderer.UseFullbrightDiffuse(textureIndex);
		else
			worldRenderer.UseDiffuseAndLightmap(textureIndex, lightmapIndex);
	}

public:
	RoomRenderer(WorldRenderer &worldRenderer, LineRenderer &lineRenderer, const Matrix &viewProjection, const bool &drawLighting):
		worldRenderer(worldRenderer),
		lineRenderer(lineRenderer),
		viewProjection(viewProjection),
		drawLighting(drawLighting)
	{}

	void Render(const GameWorld &world, uint32_t roomIndex)
	{
		const auto &room = world.rooms[roomIndex];
		const auto &internalMesh = worldRenderer.roomMeshes[roomIndex];

		//if this room is empty, we skip it
		if (internalMesh.mesh.numIndices == 0)
			return;

		//set transform
		Matrix roomTransform;
		roomTransform.SetTranslation(Vector(room.position.x, room.position.y, room.position.z));
		roomTransform.SetScale(room.scale);
		worldRenderer.SetWorldViewProjectionMatrix(viewProjection * roomTransform);

		//bind the room mesh
		worldRenderer.renderer.BindMesh<LightMappedVertex>(internalMesh.mesh);

		//for every part of the mesh...
		uint32_t startIndex = 0;
		for (const auto &part : internalMesh.parts)
		{
			SetSpecificPipeline(world, part);

			//draw the adequate number of indices
			worldRenderer.renderer.DrawBoundMesh(part.numIndices, startIndex);
			startIndex += part.numIndices;
		}
	}

	void RenderObjects(const GameWorld &world, uint32_t roomIndex)
	{
		const auto &room = world.rooms[roomIndex];

		//render room's objects
		bool drawObjects = true;
		if (drawObjects)
		{
			Matrix parentTransform;
			parentTransform.SetTranslation(Vector(room.position.x, room.position.y, room.position.z));

			ObjectRenderer objectRenderer(worldRenderer, viewProjection);
			for (Object *object = room.objects; object; object = object->next)
				objectRenderer.Render(world, object, parentTransform);
		}
	}

	void RenderTriggers(const GameWorld &world, uint32_t roomIndex)
	{
		const auto &room = world.rooms[roomIndex];

//		Matrix parentTransform;
//		parentTransform.SetTranslation(Vector(room.position.x, room.position.y, room.position.z));

		for (const auto &trigger : room.triggers)
		{
			auto bbox = BBox(
				Vector(trigger.min.x, trigger.min.y, trigger.min.z) * room.scale + Vector(room.position.x, room.position.y, room.position.z),
				Vector(trigger.max.x, trigger.max.y, trigger.max.z) * room.scale + Vector(room.position.x, room.position.y, room.position.z)
			);

			static constexpr Vector orangeColor(1.0f, 0.5f, 0.0f);
			lineRenderer.AddBox(bbox, orangeColor);
		}
	}
};

void WorldRenderer::Render(const Document &document, const Matrix &viewProjection)
{
	const GameWorld &world = document.world;

	RoomRenderer roomRenderer(*this, lineRenderer, viewProjection, document.drawLights);

	//render rooms
	if (document.drawRooms)
	{
		renderer.UsePipeline(lmsgPipeline);

		if (document.drawHelpers) //TEMP! get drawVisibleRoomsOnly
		{
			constexpr uint32_t INVALID_INDEX = 0xFFFFFFFF;

			//get the room the camera is in
			uint32_t roomIndex = INVALID_INDEX;
			Vector cameraPosition = document.camera.GetPosition();
			for (uint32_t i = 0; i < world.rooms.Count(); i++)
			{
				const auto &room = world.rooms[i];
				auto bbox = ComputeRoomBBox(room);
				bool isCameraInRoom = bbox.IsContainSphere(cameraPosition, 1.0f);
				if (isCameraInRoom)
				{
					roomIndex = i;
					break;
				}
			}

			if (roomIndex != INVALID_INDEX)
			{
				//render that room and its visible rooms
				roomRenderer.Render(world, roomIndex);
				const auto &room = world.rooms[roomIndex];
				for (const auto &visibleRoomIndex : room.viewableRooms)
					roomRenderer.Render(world, visibleRoomIndex);
			}
		}
		else
		{
			for (uint32_t i = 0; i < world.rooms.Count(); i++)
				roomRenderer.Render(world, i);
		}
	}

	//render rooms objects
	if (document.drawObjects)
	{
		renderer.UsePipeline(psgPipeline);
		for (uint32_t i = 0; i < world.rooms.Count(); i++)
			roomRenderer.RenderObjects(world, i);
	}

	//render rooms triggers
	if (document.drawTriggers)
	{
		for (uint32_t i = 0; i < world.rooms.Count(); i++)
			roomRenderer.RenderTriggers(world, i);
	}

	//render actors
//	Matrix parentTransform;
//	parentTransform.SetTranslation(Vector(room.position.x, room.position.y, room.position.z));

//	HierarchyRenderer hierarchyRenderer(renderer, viewProjection);
//	for (uint32_t i = 0; i < world.actorWAD.actors.Count(); i++)
//	if (world.actorWAD.actors.Count())
//		hierarchyRenderer.Render(world, 0); //TEMP!!!

	//render lights
#if 0
	bool drawLights = true;
	if (drawLights)
	{
		lightBulbMesh.Bind(renderer);
		for (const auto &light : world.lights)
		{
			lightBulbMesh.Draw(
				renderer,
				Vector(light.position.x, light.position.y, light.position.z),
				Vector((float)light.color[0] / 255.0f, (float)light.color[1] / 255.0f, (float)light.color[2] / 255.0f),
				viewProjection
			);
		}
	}
#endif

	//also, render the selected objects in highlight mode
	renderer.UsePipeline(csgPipeline);
	renderer.SetWorldViewProjectionMatrix(viewProjection);
	lineRenderer.Render(renderer);
}
