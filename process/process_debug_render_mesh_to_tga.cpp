#include "process.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../externals/stb_image_write.h"

void process_debug_render_mesh_to_tga(process_t& process) {
	frame_buffer_t		frame_buffer(process.render_tex_size, process.render_tex_size);
	int					len = (int)process.triangles.size();

	for (const auto &triangle : process.triangles) {
		float		*color = process.tinyobj_materials[triangle.material].diffuse;
		int			diffuse = (int)(color[0] * 255) | (int)(color[1] * 255) << 8 | (int)(color[2] * 255) << 16 | 0xff000000;
		if (triangle.valid) {
			rasterize(frame_buffer, process.points[triangle.a], process.points[triangle.b], process.points[triangle.c], diffuse);
		}
	}

	stbi_write_tga((process.file_name + ".tga").c_str(), process.render_tex_size, process.render_tex_size, 4, &(frame_buffer.pixels[0]));
}
