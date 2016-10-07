#include "process.h"

bool process_load_obj(process_t& process) {
	std::string& file_name = process.file_name;

	size_t pos = file_name.find_last_of("\\/");
	if (pos != std::string::npos) {
		process.file_path = file_name.substr(0, pos + 1);
	}

	return tinyobj::LoadObj(
		&process.tinyobj_attrib,
		&process.tinyobj_shapes,
		&process.tinyobj_materials,
		&process.tinyobj_err,
		process.file_name.c_str(),
		process.file_path.c_str());
}
