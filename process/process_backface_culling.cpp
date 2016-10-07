#include "process.h"

void process_backface_culling(process_t& process) {
	int index = process.swap_yz ? 1 : 2;
	for (auto& triangle : process.triangles) {
		triangle.valid &=
			!(process.tinyobj_attrib.normals[triangle.an * 3 + index] > 0
				&& process.tinyobj_attrib.normals[triangle.bn * 3 + index] > 0
				&& process.tinyobj_attrib.normals[triangle.cn * 3 + index] > 0);
	}
}
