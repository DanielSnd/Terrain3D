// Copyright © 2025 Cory Petkovsek, Roope Palmroos, and Contributors.

#include "core/math/vector3.h"
#include "core/math/aabb.h"
#include "core/templates/vector.h"
#include "core/templates/hash_map.h"
#include "servers/rendering_server.h"
#include "scene/resources/mesh.h"

#include "geoclipmap.h"
#include "logger.h"
#include "terrain_3d.h"

///////////////////////////
// Private Functions
///////////////////////////

// Half each triangle, have to check for longest side.
void GeoClipMap::_subdivide_half(Vector<Vector3> &vertices, Vector<int> &indices) {


	struct Vector3Hash {
		_FORCE_INLINE_ static uint32_t hash(const Vector3 &v)  {
			uint32_t hash = hash_djb2_one_float(v.x);
			hash = hash_djb2_one_float(v.y, hash);
			return hash_djb2_one_float(v.z, hash);
		}
	};

	Vector<Vector3> new_vertices;
	Vector<int> new_indices;

	HashMap<Vector3, int, Vector3Hash> vertex_map;

	auto midpoint = [](const Vector3 &p1, const Vector3 &p2) -> Vector3 {
		return (p1 + p2) / 2.0f;
	};

	auto find_or_add_vertex = [&vertex_map, &new_vertices](const Vector3 &vertex) -> int {
		int *found = vertex_map.getptr(vertex);
		if (found) {
			return *found;
		} else {
			int index = new_vertices.size();
			vertex_map[vertex] = index;
			new_vertices.push_back(vertex);
			return index;
		}
	};

	for (int i = 0; i < indices.size(); i += 3) {
		int id_0 = indices[i];
		int id_1 = indices[i + 1];
		int id_2 = indices[i + 2];

		Vector3 A = vertices[id_0];
		Vector3 B = vertices[id_1];
		Vector3 C = vertices[id_2];

		float length_AB = (B - A).length_squared();
		float length_BC = (C - B).length_squared();
		float length_CA = (A - C).length_squared();

		// Determine the longest edge and its midpoint, chaos otherwise.
		int A_id, B_id, C_id, mid_id;
		if (length_AB >= length_BC && length_AB >= length_CA) {
			A_id = find_or_add_vertex(A);
			B_id = find_or_add_vertex(B);
			C_id = find_or_add_vertex(C);
			mid_id = find_or_add_vertex(midpoint(A, B));

			new_indices.push_back(A_id);
			new_indices.push_back(mid_id);
			new_indices.push_back(C_id);

			new_indices.push_back(mid_id);
			new_indices.push_back(B_id);
			new_indices.push_back(C_id);

		} else if (length_BC >= length_AB && length_BC >= length_CA) {
			A_id = find_or_add_vertex(A);
			B_id = find_or_add_vertex(B);
			C_id = find_or_add_vertex(C);
			mid_id = find_or_add_vertex(midpoint(B, C));

			new_indices.push_back(B_id);
			new_indices.push_back(mid_id);
			new_indices.push_back(A_id);

			new_indices.push_back(mid_id);
			new_indices.push_back(C_id);
			new_indices.push_back(A_id);

		} else {
			// length_BC >= length_AB && length_BC >= length_CA
			A_id = find_or_add_vertex(A);
			B_id = find_or_add_vertex(B);
			C_id = find_or_add_vertex(C);
			mid_id = find_or_add_vertex(midpoint(C, A));

			new_indices.push_back(C_id);
			new_indices.push_back(mid_id);
			new_indices.push_back(B_id);

			new_indices.push_back(mid_id);
			new_indices.push_back(A_id);
			new_indices.push_back(B_id);
		}
	}

	vertices = new_vertices;
	indices = new_indices;
}

RID GeoClipMap::_create_mesh(const Vector<Vector3> &p_vertices, const Vector<int> &p_indices, const AABB &p_aabb) {
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = p_vertices;
	arrays[Mesh::ARRAY_INDEX] = p_indices;

	Vector<Vector3> normals;
	normals.resize(p_vertices.size());
	for (int i = 0; i < normals.size(); i++) {
		normals.write[i] = Vector3(0, 1, 0);
	}
	arrays[Mesh::ARRAY_NORMAL] = normals;

	Vector<float> tangents;
	tangents.resize(p_vertices.size() * 4);
	for (int i = 0; i < tangents.size(); i++) {
		tangents.write[i] = 0.0f;
	}
	arrays[Mesh::ARRAY_TANGENT] = tangents;

	TERRAINLOG(DEBUG, "Creating mesh via the Rendering server");
	RID mesh = RenderingServer::get_singleton()->mesh_create();
	RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh, RenderingServer::PRIMITIVE_TRIANGLES, arrays);

	TERRAINLOG(DEBUG, "Setting custom aabb: ", p_aabb.position, ", ", p_aabb.size);
	RenderingServer::get_singleton()->mesh_set_custom_aabb(mesh, p_aabb);

	return mesh;
}

///////////////////////////
// Public Functions
///////////////////////////

/* Generate clipmap meshes originally by Mike J Savage
 * Article https://mikejsavage.co.uk/blog/geometry-clipmaps.html
 * Code http://git.mikejsavage.co.uk/medfall/file/clipmap.cc.html#l197
 * In email communication with Cory, Mike clarified that the code in his
 * repo can be considered either MIT or public domain.
 */
Vector<RID> GeoClipMap::generate(const int p_size, const int p_levels) {
	TERRAINLOG(DEBUG, "Generating meshes of size: ", p_size, " levels: ", p_levels);

	// TODO bit of a mess here. someone care to clean up?
	RID tile_mesh;
	RID filler_mesh;
	RID trim_mesh;
	RID cross_mesh;
	RID seam_mesh;
	RID tile_inner_mesh;
	RID filler_inner_mesh;
	RID trim_inner_mesh;

	int TILE_RESOLUTION = p_size;
	int PATCH_VERT_RESOLUTION = TILE_RESOLUTION + 1;
	int CLIPMAP_RESOLUTION = TILE_RESOLUTION * 4 + 1;
	int CLIPMAP_VERT_RESOLUTION = CLIPMAP_RESOLUTION + 1;
	//int NUM_CLIPMAP_LEVELS = p_levels;
	AABB aabb;
	int n = 0;

	// Create a tile mesh
	// A tile is the main component of terrain panels
	// LOD0: 4 tiles are placed as a square in each center quadrant, for a total of 16 tiles
	// LOD1..N 3 tiles make up a corner, 4 corners uses 12 tiles
	{
		Vector<Vector3> vertices;
		vertices.resize(PATCH_VERT_RESOLUTION * PATCH_VERT_RESOLUTION);
		Vector<int> indices;
		indices.resize(TILE_RESOLUTION * TILE_RESOLUTION * 6);

		n = 0;

		for (int y = 0; y < PATCH_VERT_RESOLUTION; y++) {
			for (int x = 0; x < PATCH_VERT_RESOLUTION; x++) {
				vertices.write[n++] = Vector3(x, 0.f, y);
			}
		}

		n = 0;

		for (int y = 0; y < TILE_RESOLUTION; y++) {
			for (int x = 0; x < TILE_RESOLUTION; x++) {
				indices.write[n++] = _patch_2d(x, y, PATCH_VERT_RESOLUTION);
				indices.write[n++] = _patch_2d(x + 1, y + 1, PATCH_VERT_RESOLUTION);
				indices.write[n++] = _patch_2d(x, y + 1, PATCH_VERT_RESOLUTION);

				indices.write[n++] = _patch_2d(x, y, PATCH_VERT_RESOLUTION);
				indices.write[n++] = _patch_2d(x + 1, y, PATCH_VERT_RESOLUTION);
				indices.write[n++] = _patch_2d(x + 1, y + 1, PATCH_VERT_RESOLUTION);
			}
		}

		aabb = AABB(Vector3(), Vector3(PATCH_VERT_RESOLUTION, 0.1f, PATCH_VERT_RESOLUTION));
		tile_inner_mesh = _create_mesh(vertices, indices, aabb);
		_subdivide_half(vertices, indices);
		tile_mesh = _create_mesh(vertices, indices, aabb);
	}

	// Create a filler mesh
	// These meshes are small strips that fill in the gaps between LOD1+,
	// but only on the camera X and Z axes, and not on LOD0.
	{
		Vector<Vector3> vertices;
		vertices.resize(PATCH_VERT_RESOLUTION * 8);
		Vector<int> indices;
		indices.resize(TILE_RESOLUTION * 24);

		n = 0;
		int offset = TILE_RESOLUTION;

		for (int i = 0; i < PATCH_VERT_RESOLUTION; i++) {
			vertices.write[n] = Vector3(offset + i + 1.f, 0.f, 0.f);
			aabb.expand_to(vertices[n]);
			n++;

			vertices.write[n] = Vector3(offset + i + 1.f, 0.f, 1.f);
			aabb.expand_to(vertices[n]);
			n++;
		}

		for (int i = 0; i < PATCH_VERT_RESOLUTION; i++) {
			vertices.write[n] = Vector3(1.f, 0.f, offset + i + 1.f);
			aabb.expand_to(vertices[n]);
			n++;

			vertices.write[n] = Vector3(0.f, 0.f, offset + i + 1.f);
			aabb.expand_to(vertices[n]);
			n++;
		}

		for (int i = 0; i < PATCH_VERT_RESOLUTION; i++) {
			vertices.write[n] = Vector3(-real_t(offset + i), 0.f, 1.f);
			aabb.expand_to(vertices[n]);
			n++;

			vertices.write[n] = Vector3(-real_t(offset + i), 0.f, 0.f);
			aabb.expand_to(vertices[n]);
			n++;
		}

		for (int i = 0; i < PATCH_VERT_RESOLUTION; i++) {
			vertices.write[n] = Vector3(0.f, 0.f, -real_t(offset + i));
			aabb.expand_to(vertices[n]);
			n++;

			vertices.write[n] = Vector3(1.f, 0.f, -real_t(offset + i));
			aabb.expand_to(vertices[n]);
			n++;
		}

		n = 0;
		for (int i = 0; i < TILE_RESOLUTION * 4; i++) {
			int arm = i / TILE_RESOLUTION;

			int bl = (arm + i) * 2 + 0;
			int br = (arm + i) * 2 + 1;
			int tl = (arm + i) * 2 + 2;
			int tr = (arm + i) * 2 + 3;

			if (arm % 2 == 0) {
				indices.write[n++] = br;
				indices.write[n++] = bl;
				indices.write[n++] = tr;
				indices.write[n++] = bl;
				indices.write[n++] = tl;
				indices.write[n++] = tr;
			} else {
				indices.write[n++] = br;
				indices.write[n++] = bl;
				indices.write[n++] = tl;
				indices.write[n++] = br;
				indices.write[n++] = tl;
				indices.write[n++] = tr;
			}
		}
		filler_inner_mesh = _create_mesh(vertices, indices, aabb);
		_subdivide_half(vertices, indices);
		filler_mesh = _create_mesh(vertices, indices, aabb);
	}

	// Create trim mesh
	// This mesh is a skinny L shape that fills in the gaps between
	// LOD meshes when they are moving at different speeds and have gaps
	{
		Vector<Vector3> vertices;
		vertices.resize((CLIPMAP_VERT_RESOLUTION * 2 + 1) * 2);
		Vector<int> indices;
		indices.resize((CLIPMAP_VERT_RESOLUTION * 2 - 1) * 6);

		n = 0;
		Vector3 offset = Vector3(0.5f * real_t(CLIPMAP_VERT_RESOLUTION + 1), 0.f, 0.5f * real_t(CLIPMAP_VERT_RESOLUTION + 1));

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION + 1; i++) {
			vertices.write[n] = Vector3(0.f, 0.f, CLIPMAP_VERT_RESOLUTION - i) - offset;
			aabb.expand_to(vertices[n]);
			n++;

			vertices.write[n] = Vector3(1.f, 0.f, CLIPMAP_VERT_RESOLUTION - i) - offset;
			aabb.expand_to(vertices[n]);
			n++;
		}

		int start_of_horizontal = n;

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION; i++) {
			vertices.write[n] = Vector3(i + 1.f, 0.f, 0.f) - offset;
			aabb.expand_to(vertices[n]);
			n++;

			vertices.write[n] = Vector3(i + 1.f, 0.f, 1.f) - offset;
			aabb.expand_to(vertices[n]);
			n++;
		}

		n = 0;

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION; i++) {
			indices.write[n++] = (i + 0) * 2 + 1;
			indices.write[n++] = (i + 0) * 2 + 0;
			indices.write[n++] = (i + 1) * 2 + 0;

			indices.write[n++] = (i + 1) * 2 + 1;
			indices.write[n++] = (i + 0) * 2 + 1;
			indices.write[n++] = (i + 1) * 2 + 0;
		}

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION - 1; i++) {
			indices.write[n++] = start_of_horizontal + (i + 0) * 2 + 1;
			indices.write[n++] = start_of_horizontal + (i + 0) * 2 + 0;
			indices.write[n++] = start_of_horizontal + (i + 1) * 2 + 0;

			indices.write[n++] = start_of_horizontal + (i + 1) * 2 + 1;
			indices.write[n++] = start_of_horizontal + (i + 0) * 2 + 1;
			indices.write[n++] = start_of_horizontal + (i + 1) * 2 + 0;
		}
		trim_inner_mesh = _create_mesh(vertices, indices, aabb);
		_subdivide_half(vertices, indices);
		trim_mesh = _create_mesh(vertices, indices, aabb);
	}

	// Create center cross mesh
	// This mesh is the small cross shape that fills in the gaps along the
	// X and Z axes between the center quadrants on LOD0.
	{
		Vector<Vector3> vertices;
		vertices.resize(PATCH_VERT_RESOLUTION * 8);
		Vector<int> indices;
		indices.resize(TILE_RESOLUTION * 24 + 6);

		n = 0;

		for (int i = 0; i < PATCH_VERT_RESOLUTION * 2; i++) {
			vertices.write[n] = Vector3(real_t(i - TILE_RESOLUTION), 0.f, 0.f);
			aabb.expand_to(vertices[n]);
			n++;

			vertices.write[n] = Vector3(real_t(i - TILE_RESOLUTION), 0.f, 1.f);
			aabb.expand_to(vertices[n]);
			n++;
		}

		int start_of_vertical = n;

		for (int i = 0; i < PATCH_VERT_RESOLUTION * 2; i++) {
			vertices.write[n] = Vector3(0.f, 0.f, real_t(i - TILE_RESOLUTION));
			aabb.expand_to(vertices[n]);
			n++;

			vertices.write[n] = Vector3(1.f, 0.f, real_t(i - TILE_RESOLUTION));
			aabb.expand_to(vertices[n]);
			n++;
		}

		n = 0;

		for (int i = 0; i < TILE_RESOLUTION * 2 + 1; i++) {
			int bl = i * 2 + 0;
			int br = i * 2 + 1;
			int tl = i * 2 + 2;
			int tr = i * 2 + 3;

			indices.write[n++] = br;
			indices.write[n++] = bl;
			indices.write[n++] = tr;
			indices.write[n++] = bl;
			indices.write[n++] = tl;
			indices.write[n++] = tr;
		}

		for (int i = 0; i < TILE_RESOLUTION * 2 + 1; i++) {
			if (i == TILE_RESOLUTION) {
				continue;
			}

			int bl = i * 2 + 0;
			int br = i * 2 + 1;
			int tl = i * 2 + 2;
			int tr = i * 2 + 3;

			indices.write[n++] = start_of_vertical + br;
			indices.write[n++] = start_of_vertical + tr;
			indices.write[n++] = start_of_vertical + bl;
			indices.write[n++] = start_of_vertical + bl;
			indices.write[n++] = start_of_vertical + tr;
			indices.write[n++] = start_of_vertical + tl;
		}

		cross_mesh = _create_mesh(vertices, indices, aabb);
	}

	// Create seam mesh
	// This is a very thin mesh that is supposed to cover tiny gaps
	// between tiles and fillers when the vertices do not line up
	{
		Vector<Vector3> vertices;
		vertices.resize(CLIPMAP_VERT_RESOLUTION * 4);
		Vector<int> indices;
		indices.resize(CLIPMAP_VERT_RESOLUTION * 6);

		n = 0;

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION; i++) {
			n = CLIPMAP_VERT_RESOLUTION * 0 + i;
			vertices.write[n] = Vector3(i, 0.f, 0.f);
			aabb.expand_to(vertices[n]);

			n = CLIPMAP_VERT_RESOLUTION * 1 + i;
			vertices.write[n] = Vector3(CLIPMAP_VERT_RESOLUTION, 0.f, i);
			aabb.expand_to(vertices[n]);

			n = CLIPMAP_VERT_RESOLUTION * 2 + i;
			vertices.write[n] = Vector3(CLIPMAP_VERT_RESOLUTION - i, 0.f, CLIPMAP_VERT_RESOLUTION);
			aabb.expand_to(vertices[n]);

			n = CLIPMAP_VERT_RESOLUTION * 3 + i;
			vertices.write[n] = Vector3(0.f, 0.f, CLIPMAP_VERT_RESOLUTION - i);
			aabb.expand_to(vertices[n]);
		}

		n = 0;

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION * 4; i += 2) {
			indices.write[n++] = i + 1;
			indices.write[n++] = i;
			indices.write[n++] = i + 2;
		}

		indices.write[indices.size() - 1] = 0;

		seam_mesh = _create_mesh(vertices, indices, aabb);
	}

	// skirt mesh
	/*{
		real_t scale = real_t(1 << (NUM_CLIPMAP_LEVELS - 1));
		real_t fbase = real_t(TILE_RESOLUTION << NUM_CLIPMAP_LEVELS);
		Vector2 base = -Vector2(fbase, fbase);

		Vector2 clipmap_tl = base;
		Vector2 clipmap_br = clipmap_tl + (Vector2(CLIPMAP_RESOLUTION, CLIPMAP_RESOLUTION) * scale);

		real_t big = 10000000.0;
		Array vertices = Array::make(
			Vector3(-1, 0, -1) * big,
			Vector3(+1, 0, -1) * big,
			Vector3(-1, 0, +1) * big,
			Vector3(+1, 0, +1) * big,
			Vector3(clipmap_tl.x, 0, clipmap_tl.y),
			Vector3(clipmap_br.x, 0, clipmap_tl.y),
			Vector3(clipmap_tl.x, 0, clipmap_br.y),
			Vector3(clipmap_br.x, 0, clipmap_br.y)
		);

		Array indices = Array::make(
			0, 1, 4, 4, 1, 5,
			1, 3, 5, 5, 3, 7,
			3, 2, 7, 7, 2, 6,
			4, 6, 0, 0, 6, 2
		);

		skirt_mesh = _create_mesh(PackedVector3Array(vertices), PackedInt32Array(indices));

	}*/

	Vector<RID> meshes = {
		tile_mesh,
		filler_mesh,
		trim_mesh,
		cross_mesh,
		seam_mesh,
		tile_inner_mesh,
		filler_inner_mesh,
		trim_inner_mesh,
	};

	return meshes;
}
