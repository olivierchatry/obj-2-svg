#include "process.h"

void process_triangle_occlusion_null(process_t& process) {
	for (auto& triangle : process.triangles) {
		triangle.valid = 1;
	}
}

void process_triangle_occlusion(process_t& process) {
	frame_buffer_t		frame_buffer(process.render_tex_size, process.render_tex_size);
	int					len = (int)process.triangles.size();

	for (int i = 0; i < len; ++i) {
		auto& triangle = process.triangles[i];
		rasterize(frame_buffer, process.points[triangle.a], process.points[triangle.b], process.points[triangle.c], i);
	}
	for (auto& triangle : process.triangles) {
		triangle.max_z = -std::numeric_limits<float>::max();
		triangle.valid = 0;
	}

	for (int i = 0; i < frame_buffer.width * frame_buffer.height; ++i) {
		int index = frame_buffer.pixels[i];
		if (index != -1) {
			auto& triangle = process.triangles[index];
			triangle.valid = 1;
			triangle.max_z = std::max(triangle.max_z, frame_buffer.depths[i]);
		}
	}
}
