#include "process.h"


void process_compute_min_max(process_t& process) {
	process.min.set(std::numeric_limits<float>::max());
	process.max.set(-std::numeric_limits<float>::max());

	float* vertices_start = &(process.tinyobj_attrib.vertices[0]);
	float* vertices_end = vertices_start + process.tinyobj_attrib.vertices.size();
	process.vertices.reserve(process.tinyobj_attrib.vertices.size() / 3);
	if (process.swap_yz) {
		for (float* vertex = vertices_start; vertex != vertices_end; vertex += 3) {
			std::swap(vertex[1], vertex[2]);
		}
	}

	for (float* vertex = vertices_start; vertex != vertices_end; vertex += 3) {
		v3 v(vertex);
		process.vertices.push_back(v);
		process.min.min(v);
		process.max.max(v);
	}
}
