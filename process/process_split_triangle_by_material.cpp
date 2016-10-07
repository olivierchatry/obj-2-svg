#include "process.h"

void process_split_triangle_by_material(process_t& process) {
	for (const auto& triangle : process.triangles) {
		if (triangle.valid) {
			process.triangles_per_materials[triangle.material].push_back(triangle);
		}
	}
}