#include "process.h"

void process_remove_degenerate_triangle(process_t& process) {	
	process.triangles.clear();
	for (auto& triangle_for_material : process.triangles_per_materials) {
		for (auto& triangle : triangle_for_material.second) {
			if (
				triangle.a == triangle.b
				|| triangle.a == triangle.c
				|| triangle.b == triangle.c) {
				triangle.valid = 0;
			}
			else {
				process.triangles.push_back(triangle);
			}
		}
	}
}

