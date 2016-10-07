#include "process.h"

void process_generate_triangle_list(process_t& process) {
	size_t triangle_count = 0;

	for (const auto& shape : process.tinyobj_shapes) {
		if (shape.name.find("tag_") != 0) {
			triangle_count += shape.mesh.num_face_vertices.size();
		}
	}

	process.triangles.resize(triangle_count);
	size_t triangle_index = 0;
	for (const auto& shape : process.tinyobj_shapes) {
		if (shape.name.find("tag_") == 0) {
			continue;
		}

		size_t faces_count = shape.mesh.num_face_vertices.size();
		size_t offset = 0;
		const auto& attrib = process.tinyobj_attrib;

		for (size_t face = 0; face < faces_count; ++face) {
			int num_face_vertices = shape.mesh.num_face_vertices[face];
			int material = shape.mesh.material_ids[face];

			if (num_face_vertices == 3) {
				auto& t1 = process.triangles[triangle_index++];
				t1.a = shape.mesh.indices[offset + 0].vertex_index;
				t1.b = shape.mesh.indices[offset + 1].vertex_index;
				t1.c = shape.mesh.indices[offset + 2].vertex_index;

				t1.an = shape.mesh.indices[offset + 0].normal_index;
				t1.bn = shape.mesh.indices[offset + 1].normal_index;
				t1.cn = shape.mesh.indices[offset + 2].normal_index;

				t1.max_z = std::max(attrib.vertices[t1.a + 2], std::max(attrib.vertices[t1.b + 2], attrib.vertices[t1.c + 2]));
				t1.material = material;
				t1.valid = 1;
			}
			else if (num_face_vertices == 4) {
				auto& t1 = process.triangles[triangle_index++];
				t1.a = shape.mesh.indices[offset + 0].vertex_index;
				t1.b = shape.mesh.indices[offset + 1].vertex_index;
				t1.c = shape.mesh.indices[offset + 3].vertex_index;

				t1.an = shape.mesh.indices[offset + 0].normal_index;
				t1.bn = shape.mesh.indices[offset + 1].normal_index;
				t1.cn = shape.mesh.indices[offset + 3].normal_index;

				t1.max_z = std::max(attrib.vertices[t1.a + 2], std::max(attrib.vertices[t1.b + 2], attrib.vertices[t1.c + 2]));
				t1.material = material;
				t1.valid = 1;

				auto& t2 = process.triangles[triangle_index++];
				t2.a = shape.mesh.indices[offset + 1].vertex_index;
				t2.b = shape.mesh.indices[offset + 2].vertex_index;
				t2.c = shape.mesh.indices[offset + 3].vertex_index;

				t2.an = shape.mesh.indices[offset + 1].normal_index;
				t2.bn = shape.mesh.indices[offset + 2].normal_index;
				t2.cn = shape.mesh.indices[offset + 3].normal_index;

				t2.max_z = std::max(attrib.vertices[t2.a + 2], std::max(attrib.vertices[t2.b + 2], attrib.vertices[t2.c + 2]));
				t2.material = material;
				t2.valid = 1;
			}
			offset += num_face_vertices;
		}
	}

	std::sort(
		process.triangles.begin(),
		process.triangles.end(),
		[](const triangle_t& a, triangle_t& b) -> bool { return a.max_z > b.max_z; }
	);
}
