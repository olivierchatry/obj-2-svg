#include "process.h"

void process_transform_points(process_t& process) {
	const auto& attrib = process.tinyobj_attrib;

	size_t len = attrib.vertices.size();

	process.points.resize(len / 3);

	float width = process.max.x - process.min.x;
	float height = process.max.y - process.min.y;

	for (size_t i = 0, j = 0; i < len; i += 3, j++) {
		const float*		xyz = &(attrib.vertices[i]);
		point_t&				p = process.points[j];

		p.x = (int)(0.5f + ((xyz[0] - process.min.x) / width) * process.render_tex_size);
		p.y = (int)(0.5f + ((xyz[1] - process.min.y) / height) * process.render_tex_size);
		p.z = xyz[2];
	}
}
