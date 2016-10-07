#include "process.h"

void process_optimize_mesh(process_t& process) {
	std::vector<v3>					remaped_vertices;
	std::vector<point_t>			remaped_points;

	remaped_points.reserve(process.points.size());
	remaped_vertices.reserve(process.tinyobj_attrib.vertices.size() / 3);

	for (auto& triangle_for_material : process.triangles_per_materials) {
		std::map<point_t, int>		point_to_index;

		auto& triangles = triangle_for_material.second;
		triangles_t	remaped_triangles;
		size_t				len = triangles.size();
		remaped_triangles.resize(len);

		for (size_t i = 0; i < len; ++i) {
			const auto& triangle = triangles[i];
			auto& remaped_triangle = remaped_triangles[i];

			auto		remap = [&](int index) -> int {
				const auto& p = process.points[index];
				const auto it = point_to_index.find(p);
				int remap_index = 0;

				if (it == point_to_index.end()) {
					remap_index = (int)remaped_vertices.size();

					v3 v(&(process.tinyobj_attrib.vertices[index * 3]));
					remaped_vertices.push_back(v);
					remaped_points.push_back(p);

					point_to_index.insert(
						std::make_pair(p, remap_index)
					);
				}
				else {
					remap_index = it->second;
				}
				return remap_index;
			};
			remaped_triangle.material = triangle.material;
			remaped_triangle.max_z = triangle.max_z;
			remaped_triangle.valid = triangle.valid;

			remaped_triangle.a = remap(triangle.a);
			remaped_triangle.b = remap(triangle.b);
			remaped_triangle.c = remap(triangle.c);
		}
		triangles.swap(remaped_triangles);
	}
	process.points.swap(remaped_points);
	process.vertices.swap(remaped_vertices);
}
