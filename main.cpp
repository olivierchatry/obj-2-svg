#include "process/process.h"

int main(int ac, char** av) {
	for (int i = 1; i < ac; ++i) {
		process_t process;
		process.file_name = av[i];
		if (!process_load_obj(process)) {
			continue;
		}
		process.swap_yz = true;
		process.render_tex_size = 4096;

		process_compute_min_max(process);		
		process_transform_points(process);
		process_generate_triangle_list(process);
		// process_backface_culling(process);
		// triangle_occlusion_null(process);
		
		process_triangle_occlusion(process);			
				
		process_split_triangle_by_material(process);
		
		// optimize_mesh(process);		
		process_remove_degenerate_triangle(process);
		process_debug_render_mesh_to_tga(process);

		process_output_svg(process);
	}
}
