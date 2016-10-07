#pragma once
#include <vector>
#include <limits>
#include <algorithm>
#include <math.h>


struct point_t {
	int		x, y;
	float z;

	bool operator<(const point_t &that)  const {
		if (x != that.x) {
			return x < that.x;
		} else if (y != that.y) {
			return y < that.y;
		} else if (fabsf(z - that.z) < std::numeric_limits<float>::epsilon() * 2.0f) {
			return z < that.z;
		}
		return false;
	}
};


struct frame_buffer_t {

	frame_buffer_t(int w, int h) : width(w), height(h) {
		pixels.resize(w * h, -1);
		depths.resize(w * h, -std::numeric_limits<float>::max());
	}

	int width, height;

	std::vector<int>			pixels;
	std::vector<float>		depths;
};

// Rendering code largely inspired from Intel code : 
// https://software.intel.com/en-us/blogs/2013/03/22/software-occlusion-culling-update
// And ryg optimization ( https://github.com/rygorous/intel_occlusion_cull )

inline void rasterize(frame_buffer_t& fb, const point_t& v0, const point_t& v1, const point_t& v2, int value) {
	// Compute triangle bounding box
	int minX = std::min(v0.x, std::min(v1.x, v2.x));
	int minY = std::min(v0.y, std::min(v1.y, v2.y));

	int maxX = std::min(std::max(v0.x, std::max(v1.x, v2.x)) + 1, fb.width);
	int maxY = std::min(std::max(v0.y, std::max(v1.y, v2.y)) + 1, fb.height);

	int A0 = v1.y - v2.y;
	int A1 = v2.y - v0.y;
	int A2 = v0.y - v1.y;

	int B0 = v2.x - v1.x;
	int B1 = v0.x - v2.x;
	int B2 = v1.x - v0.x;

	int C0 = v1.x * v2.y - v2.x * v1.y;
	int C1 = v2.x * v0.y - v0.x * v2.y;
	int C2 = v0.x * v1.y - v1.x * v0.y;

	int		triArea = (v1.x - v0.x) * (v2.y - v0.y) - (v0.x - v2.x) * (v0.y - v1.y);
	float	oneOverTriArea = (1.0f / float(triArea));

	float	Z[3] = {
		v0.z,
		(v1.z - v0.z) * oneOverTriArea,
		(v2.z - v0.z) * oneOverTriArea
	};

	int		alpha0= (A0 * minX) + (B0 * minY) + C0;
	int		beta0 = (A1 * minX) + (B1 * minY) + C1;
	int		gama0 = (A2 * minX) + (B2 * minY) + C2;
	float	zx			= A1 * Z[1] + A2 * Z[2];
	
	int		rowIdx = (minY * fb.width + minX);
	for (int y = minY; y < maxY; y++) {
		int		alpha = alpha0, beta = beta0, gama = gama0;
		float	depth = Z[0] + Z[1] * beta + Z[2] * gama;
		int		index = rowIdx;
		for (int x = minX; x < maxX; x++) {
			if ( ((alpha | beta | gama) >= 0) && depth > fb.depths[index]) {
					fb.depths[index] = depth;
					fb.pixels[index] = value;
			}			
			alpha += A0, beta += A1, gama += A2, depth += zx, index++;
		}
		alpha0 += B0, beta0 += B1, gama0 += B2, rowIdx += fb.width;
	}

}