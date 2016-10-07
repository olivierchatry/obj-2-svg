#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <iostream>

#include <vector>
#include <list>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <limits>
#include <cstdio>

#include <vector>

#include "../externals/tiny_obj_loader.h"
#include "../libs/v3.h"
#include "../libs/rasterizer.h"


struct triangle_t {
	int			a, b, c;
	int			an, bn, cn;
	float		max_z;
	int			material;
	int			valid;
};

typedef std::vector<triangle_t>	triangles_t;

struct process_t {
	std::vector<tinyobj::material_t>		tinyobj_materials;
	std::vector<tinyobj::shape_t>			tinyobj_shapes;
	tinyobj::attrib_t									tinyobj_attrib;
	std::string												tinyobj_err;

	int																render_tex_size;

	triangles_t												triangles;
	std::vector<point_t>								points;
	std::vector<v3>										vertices;

	std::string												file_name;
	std::string												file_path;
	bool																swap_yz;

	v3																	min;
	v3																	max;

	std::map<int, triangles_t>				triangles_per_materials;
};

inline void get_diffuse_from_tinyobj_material(process_t& process, int material, char* buffer) {
	float *diffuse = process.tinyobj_materials[material].diffuse;
	int			diffuse_as_int[3] = {
		(int)(diffuse[0] * 255),
		(int)(diffuse[1] * 255),
		(int)(diffuse[2] * 255),
	};
	std::sprintf(buffer, "#%0.2X%0.2X%0.2X", diffuse_as_int[0], diffuse_as_int[1], diffuse_as_int[2]);
}

void process_optimize_mesh(process_t& process);
void process_triangle_occlusion_null(process_t& process);
void process_triangle_occlusion(process_t& process);
void process_remove_degenerate_triangle(process_t& process);
void process_debug_render_mesh_to_tga(process_t& process);
void process_output_svg(process_t& process);
void process_compute_min_max(process_t& process);
void process_transform_points(process_t& process);
void process_split_triangle_by_material(process_t& process);
void process_generate_triangle_list(process_t& process);
bool process_load_obj(process_t& process);