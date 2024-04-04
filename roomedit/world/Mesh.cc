/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "Mesh.hh"
#include "common.hh"

static void ReadRecursiveSec(ReadStream &rs)
{
	char bRead;
	rs >> bRead;
	if (bRead)
	{
		rs.AdvanceBy(9);

		short unk10;
		rs >> unk10;
		if (unk10)
			rs.AdvanceBy((size_t)unk10 * 2);

		/* dwa razy! */
		ReadRecursiveSec(rs);
		ReadRecursiveSec(rs);
	}
}

static Face LoadFace(ReadStream &rs, MemoryPool &pool)
{
	Face face;

	rs >> face.indexSurfaceProperty;
	rs >> face.lightmapIndex;
	rs >> face.typePoly;

	rs >> face.numVerts;
	face.vertexIndices = std::move(pool.CreateArray<unsigned short>(face.numVerts));
	rs.Read(face.vertexIndices.Data(), face.numVerts * sizeof(unsigned short)); //read directly

	uint16_t numI;
	rs >> numI;
	face.mi = std::move(pool.CreateArray<int>(numI));
	rs.Read(face.mi.Data(), numI * sizeof(int)); //read directly

	//load corner indices
	uint32_t numGlobalI;
	rs >> numGlobalI;
	face.globalI = std::move(pool.CreateArray<short>(numGlobalI));
	rs.Read(face.globalI.Data(), numGlobalI * sizeof(short)); //read directly

	//load hull
	uint32_t num_unk4;
	rs >> num_unk4;
	if (num_unk4 == 0)
	{
		rs >> face.hull.charC;
		rs >> face.hull.vec0;
		rs >> face.hull.vec1;
		rs >> face.hull.normal;
		rs >> face.hull.vec3;

		uint32_t numVertices;
		rs >> numVertices;
		face.hull.vertices = std::move(pool.CreateArray<Vector3>(numVertices));
		rs.Read(face.hull.vertices.Data(), numVertices * sizeof(Vector3)); //read directly
		for (auto &hullVert : face.hull.vertices)
			ConvertHandedness(hullVert);

		//check validity of that hull
		face.hull.valid = face.hull.charC < 0 || face.indexSurfaceProperty != 3;
	}

	rs >> face.normal;
	rs >> face.minExtent;
	ConvertHandedness(face.minExtent);
	rs >> face.maxExtent;
	ConvertHandedness(face.maxExtent);

	bool unk5;
	rs >> unk5;
	if (unk5)
	{
		short unk6, unk7;
		rs >> unk6;
		rs >> unk7;
		rs.AdvanceBy((size_t)unk6 * 2);
		rs.AdvanceBy((size_t)unk7 * 2);

		short unk8;
		rs >> unk8;
		rs.AdvanceBy((size_t)unk8 * 4);
	}

	return face;
}

Mesh ReadMesh(ReadStream &rs, MemoryPool &pool)
{
	Mesh mesh;

	LoadDrawableName(rs, mesh.name);

	rs >> mesh.flags;

	rs >> mesh.numVerts;
	assert(mesh.numVerts <= 8000); //sanity check
	if (mesh.numVerts) //it is possible that a Room does not have any vertices
	{
		uint32_t size = mesh.numVerts * sizeof(Vector3);
		mesh.positions = pool.Allocate<Vector3>(mesh.numVerts);
		rs.Read(mesh.positions, size);
		for (uint32_t i = 0; i < mesh.numVerts; i++)
			ConvertHandedness(mesh.positions[i]);
		mesh.normals = pool.Allocate<Vector3>(mesh.numVerts);
		rs.Read(mesh.normals, size);
		for (uint32_t i = 0; i < mesh.numVerts; i++)
			ConvertHandedness(mesh.normals[i]);
	}

	rs.AdvanceBy(4);

	rs >> mesh.minExtent;
//	ConvertHandedness(mesh.minExtent);
	rs >> mesh.maxExtent;
//	ConvertHandedness(mesh.maxExtent);
//	RecomputeExtents(mesh.minExtent, mesh.maxExtent);

	uint32_t numCorners;
	rs >> numCorners;
	assert(numCorners <= 4500); //sanity check
	if (numCorners == 0)
		rs.AdvanceBy(4);
	else
	{
		mesh.corners = std::move(pool.CreateArray<Corner>(numCorners));
		rs.Read(mesh.corners.Data(), numCorners * sizeof(Corner)); //read directly

		uint32_t numFaces;
		rs >> numFaces;
		assert(numFaces <= 2000); //sanity check
		mesh.faces = std::move(pool.CreateArray<Face>(numFaces));
		for (Face &face : mesh.faces)
			face = std::move(LoadFace(rs, pool));
	}

	int numCornersTextureAxis;
	rs >> numCornersTextureAxis;
	struct TEXTURE_AXIS
	{
		Vector3 axis_u;
		Vector3 axis_v;
		Vector3 axis_w;
	};
	rs.AdvanceBy(numCornersTextureAxis * sizeof(TEXTURE_AXIS));
	//TODO: ConvertHandedness

	bool hasLocator;
	rs >> hasLocator;
	if (hasLocator)
		rs.AdvanceBy(24);

	//HIT_DATA
	short unk5;
	rs >> unk5;
	if (unk5)
		ReadRecursiveSec(rs);

	return mesh;
}
