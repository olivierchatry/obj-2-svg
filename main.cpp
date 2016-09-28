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

#include "tiny_obj_loader.h"
#include "rasterizer.h"
#include "v3.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

bool load_obj(const std::string& file_name, process_t& process) {
	size_t pos = file_name.find_last_of("\\/");	
	if (pos != std::string::npos) {
		process.file_path = file_name.substr(0, pos + 1);
	}
	process.file_name = file_name;
	
	return tinyobj::LoadObj(
		&process.tinyobj_attrib, 
		&process.tinyobj_shapes, 
		&process.tinyobj_materials, 
		&process.tinyobj_err, 
		process.file_name.c_str(), 
		process.file_path.c_str());	
}

void generate_triangle_list(process_t& process) {
	size_t triangle_count = 0;
	
	for (const auto& shape : process.tinyobj_shapes) {
		if (shape.name.find("tag_") != 0) {
			triangle_count += shape.mesh.num_face_vertices.size();
		}
	}

	process.triangles.resize(triangle_count);
	size_t triangle_index = 0;
	for (const auto& shape : process.tinyobj_shapes) {
		if (shape.name.find("tag_") == 0) {
			continue;
		}

		size_t faces_count = shape.mesh.num_face_vertices.size();
		size_t offset = 0;
		const auto& attrib = process.tinyobj_attrib;

		for (size_t face = 0; face < faces_count; ++face) {
			int num_face_vertices = shape.mesh.num_face_vertices[face];
			int material = shape.mesh.material_ids[face];

			if (num_face_vertices == 3) {
				auto& t1 = process.triangles[triangle_index++];
				t1.a = shape.mesh.indices[offset + 0].vertex_index;
				t1.b = shape.mesh.indices[offset + 1].vertex_index;
				t1.c = shape.mesh.indices[offset + 2].vertex_index;

				t1.an = shape.mesh.indices[offset + 0].normal_index;
				t1.bn = shape.mesh.indices[offset + 1].normal_index;
				t1.cn = shape.mesh.indices[offset + 2].normal_index;

				t1.max_z = std::max(attrib.vertices[t1.a + 2], std::max(attrib.vertices[t1.b + 2], attrib.vertices[t1.c + 2]));
				t1.material = material;
				t1.valid = 1;
			} else if (num_face_vertices == 4) {
				auto& t1 = process.triangles[triangle_index++];
				t1.a = shape.mesh.indices[offset + 0].vertex_index;
				t1.b = shape.mesh.indices[offset + 1].vertex_index;
				t1.c = shape.mesh.indices[offset + 3].vertex_index;

				t1.an = shape.mesh.indices[offset + 0].normal_index;
				t1.bn = shape.mesh.indices[offset + 1].normal_index;
				t1.cn = shape.mesh.indices[offset + 3].normal_index;

				t1.max_z = std::max(attrib.vertices[t1.a + 2], std::max(attrib.vertices[t1.b + 2], attrib.vertices[t1.c + 2]));
				t1.material = material;
				t1.valid = 1;

				auto& t2 = process.triangles[triangle_index++];
				t2.a = shape.mesh.indices[offset + 1].vertex_index;
				t2.b = shape.mesh.indices[offset + 2].vertex_index;
				t2.c = shape.mesh.indices[offset + 3].vertex_index;
				
				t2.an = shape.mesh.indices[offset + 1].normal_index;
				t2.bn = shape.mesh.indices[offset + 2].normal_index;
				t2.cn = shape.mesh.indices[offset + 3].normal_index;

				t2.max_z = std::max(attrib.vertices[t2.a + 2], std::max(attrib.vertices[t2.b + 2], attrib.vertices[t2.c + 2]));
				t2.material = material;
				t2.valid = 1;
			}
			offset += num_face_vertices;
		}
	}

	std::sort(
		process.triangles.begin(), 
		process.triangles.end(), 
		[](const triangle_t& a, triangle_t& b) -> bool { return a.max_z > b.max_z; }
	);
}

void backface_culling(process_t& process) {
	int index = process.swap_yz ? 1 : 2;
	for (auto& triangle : process.triangles) {
		triangle.valid &= 
			!(process.tinyobj_attrib.normals[triangle.an * 3 + index] > 0
			&& process.tinyobj_attrib.normals[triangle.bn * 3 + index] > 0
			&& process.tinyobj_attrib.normals[triangle.cn * 3 + index] > 0);
	}
}

void compute_min_max(process_t& process) {
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

void transform_points(process_t& process) {
	const auto& attrib = process.tinyobj_attrib;

	size_t len = attrib.vertices.size();
	
	process.points.resize(len / 3);
	
	float width		= process.max.x - process.min.x;
	float height		= process.max.y - process.min.y;
	
	for (size_t i = 0, j = 0; i < len; i += 3, j++) {
		const float*		xyz = &(attrib.vertices[i]);
		point_t&				p = process.points[j];
	
		p.x = (int)(0.5f + ((xyz[0] - process.min.x) / width) * process.render_tex_size);
		p.y = (int)(0.5f + ((xyz[1] - process.min.y) / height) * process.render_tex_size);
		p.z = xyz[2];
	}
}

void triangle_occlusion_null(process_t& process) {
	for (auto& triangle : process.triangles) {		
		triangle.valid = 1;
	}
}

void triangle_occlusion(process_t& process) {
	frame_buffer_t		frame_buffer(process.render_tex_size, process.render_tex_size);
	int					len = (int) process.triangles.size();
	
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

void render_mesh(process_t& process) {
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


void split_triangle_by_material(process_t& process) {	
	for (const auto& triangle : process.triangles) {
		if (triangle.valid) {
			process.triangles_per_materials[triangle.material].push_back(triangle);
		}
	}
}


void optimize_mesh(process_t& process) {
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
				} else {
					remap_index = it->second;
				}
				return remap_index;
			};
			remaped_triangle.material = triangle.material;
			remaped_triangle.max_z= triangle.max_z;
			remaped_triangle.valid = triangle.valid;
			
			remaped_triangle.a = remap(triangle.a);
			remaped_triangle.b = remap(triangle.b);
			remaped_triangle.c = remap(triangle.c);
		}			
		triangles.swap(remaped_triangles);
	}
	std::cout << process.points.size() << " --> " << remaped_points.size() << std::endl;
	process.points.swap(remaped_points);
	process.vertices.swap(remaped_vertices);
}

void remove_colinear_triangle(process_t& process) {
	std::cout << process.triangles.size() << " --> ";
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
	std::cout << process.triangles.size() << std::endl;
}

void get_diffuse_from_tinyobj_material(process_t& process, int material, char* buffer) {
	float *diffuse = process.tinyobj_materials[material].diffuse;
	int			diffuse_as_int[3] = {
		(int)(diffuse[0] * 255),
		(int)(diffuse[1] * 255),
		(int)(diffuse[2] * 255),
	};
	std::sprintf(buffer, "#%0.2X%0.2X%0.2X", diffuse_as_int[0], diffuse_as_int[1], diffuse_as_int[2]);
}

void output_svg(process_t& process) {
	struct edge_t {
		int material;
		int count;
	};

	std::map<std::pair<int, int>, edge_t>	edges;

	for (auto triangle : process.triangles) {
		if (triangle.valid) {
			auto addEdge = [&](int a, int b, int material) {
				if (a > b) {
					std::swap(a, b);
				}
				auto& e = edges[std::make_pair(a, b)];
				e.count++;
				e.material = material;
			};
			addEdge(triangle.a, triangle.b, triangle.material);
			addEdge(triangle.b, triangle.c, triangle.material);
			addEdge(triangle.c, triangle.a, triangle.material);
		}
	}

	auto iter = edges.begin(), end = edges.end();
	while (iter != end) {
		if (iter->second.count > 1) {
			edges.erase(iter++);
		}
		else {
			++iter;
		}
	}


	iter = edges.begin();
	struct entry_t {
		int material;
		float z;
		int id;
		std::vector<int> indices;
		std::vector<v3>	vertices;
	};
	std::vector<entry_t> entries;

	while (iter != end) {
		std::vector<int> stack;

		int material = iter->second.material;
		stack.push_back( iter->first.first );
		stack.push_back( iter->first.second );
		edges.erase(iter);
		if (entries.size() == 1) {
			// __debugbreak();
		}
		int found = 1;

		while (found) {
			int		to_find = stack.back();
			auto		it = std::find_if(edges.begin(), edges.end(), [=](auto& e) -> bool { return to_find == e.first.first || to_find == e.first.second; });
			found = it != edges.end();
			if (found) {
				stack.push_back(it->first.first == to_find ? it->first.second : it->first.first);
				edges.erase(it);
			}
		}
		stack.pop_back();
		
		entries.push_back(entry_t());
		entry_t& entry = entries.back();
		entry.id = entries.size();
		entry.material = material;
		entry.z = -std::numeric_limits<float>::max();
		for (auto eit : stack) {
			const auto& a = process.vertices[eit];
			entry.indices.push_back(eit);
			entry.vertices.push_back(a);
			entry.z = std::max(entry.z, a.z);
		}

		iter = edges.begin();
	}

	std::ofstream svg;
	svg.open(process.file_name + ".svg");
	svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" << process.min.x << " " << process.min.y << " " << (process.max.x - process.min.x) << " " << (process.max.y - process.min.y) << "\" >";

	std::sort(entries.begin(), entries.end(), [](auto a, auto b) { return a.z < b.z; });
	int previous_material = -1;
	int id = 0;
	for (auto entry : entries) {
		int need_change_material = previous_material != entry.material;
		
		if (need_change_material) {
			if (previous_material != -1) {
				svg << "</g>";
			}			
			previous_material = entry.material;
			char color[16];
			get_diffuse_from_tinyobj_material(process, entry.material, color);
			svg << "<g id=\"" << id << "\" stroke=\"black\" fill=\"" << color << "\" stroke-width=\"0.01\">";
			id++;
		}
		svg << "<polygon points=\"";
		for (auto a : entry.vertices) {
			svg << a.x << "," << a.y << " ";
		}
		svg << "\"/>";
	}
	svg << "</g></svg>";
}

int main(int ac, char** av) {
	for (int i = 1; i < ac; ++i) {
		process_t process;

		if (!load_obj(av[i], process)) {
			continue;
		}

		process.swap_yz = true;
		process.render_tex_size = 4096;

		compute_min_max(process);		
		transform_points(process);
		generate_triangle_list(process);		
		// backface_culling(process);
		// triangle_occlusion_null(process);
		
		triangle_occlusion(process);
			
		
		
		split_triangle_by_material(process);
		
		// optimize_mesh(process);		
		remove_colinear_triangle(process);		
		render_mesh(process);

		output_svg(process);
	}
}

	//	std::ofstream svg;

	//	svg.open(fileName + ".svg");
	//	svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" << fb.mx << " " << fb.my << " " << (fb.Mx - fb.mx) << " " << (fb.My - fb.my) << "\" preserveAspectRatio=\"xMidYmin meet\" >" << std::endl;

	//	//
	//	std::map<int, std::vector<triangle_t> > indices_per_materials;
	//	for (const auto& triangle : triangles) {
	//		if (!triangle.valid) {
	//			continue;
	//		}
	//		indices_per_materials[triangle.material].push_back(triangle);
	//	}

	//	// optimize mesh.

	//	for (const auto& indices_per_material : indices_per_materials) {
	//		std::map<std::pair<int, int>, int>		edges;
	//		const auto		&triangles = indices_per_material.second;
	//		float *diffuse = materials[indices_per_material.first].diffuse;
	//		int			diffuse_as_int[3] = {
	//			(int)(diffuse[0] * 255),
	//			(int)(diffuse[1] * 255),
	//			(int)(diffuse[2] * 255),
	//		};
	//		char 	color[16] = { 0 };
	//		std::snprintf(color, 16, "#%0.2X%0.2X%0.2X", diffuse_as_int[0], diffuse_as_int[1], diffuse_as_int[2]);

	//		size_t				indices_count = triangles.size();
	//		for (size_t i = 0; i < indices_count; i++) {
	//			int a = triangles[i].a;
	//			int b = triangles[i].b;
	//			int c = triangles[i].c;
	//			auto addEdge = [](auto a, auto b, auto& output) {
	//				if (a > b) {
	//					output[std::pair<int, int>(b, a)] ++;
	//				}
	//				else {
	//					output[std::pair<int, int>(a, b)] ++;
	//				}
	//			};
	//			addEdge(a, b, edges);
	//			addEdge(b, c, edges);
	//			addEdge(c, a, edges);
	//		}
	//		auto iter = edges.begin(), end = edges.end();
	//		while (iter != end) {
	//			if (iter->second > 1) {
	//				edges.erase(iter++);
	//			}
	//			else {
	//				++iter;
	//			}
	//		}
	//		iter = edges.begin();
	//		while (iter != end) {
	//			std::vector<int> stack;
	//			stack.push_back(iter->first.first);
	//			stack.push_back(iter->first.second);
	//			edges.erase(iter);
	//			int found = 1;

	//			while (found) {
	//				int to_find = stack.back();
	//				auto it = std::find_if(edges.begin(), edges.end(), [=](auto& e) -> bool { return to_find == e.first.first || to_find == e.first.second; });
	//				found = it != edges.end();
	//				if (found) {
	//					stack.push_back(it->first.first == to_find ? it->first.second : it->first.first);
	//					edges.erase(it);
	//				}
	//			}
	//			if (stack.back() != stack.front()) {
	//				std::cout << "err" << std::endl;
	//			}
	//			stack.pop_back();

	//			svg << "<polygon stroke=\"black\" stroke-width=\"0.01\" points=\"";
	//			for (auto eit : stack) {
	//				float* a = &(attrib.vertices[eit]);
	//				svg << a[0] << "," << a[1] << " ";
	//			}
	//			svg << "\" fill=\"" << color << "\""
	//				<< "/>" << std::endl;

	//			iter = edges.begin();
	//		}

	//	}
	//	svg << "</svg>" << std::endl;
	//}
//}
