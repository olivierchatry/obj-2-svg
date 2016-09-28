#pragma once

#include <algorithm>

struct v3 {
public:
	float x, y, z;

public:
	v3() {
	}

	v3(float v) {
		set(v);
	}

	v3(const v3& that) {
		x = that.x;
		y = that.y;
		z = that.z;
	}
	v3(float* av) {
		set(av);
	}

	v3(float x_, float y_, float z_) {
		set(x_, y_, z_);
	}

	inline void min(const v3& that) {
		x = std::min(x, that.x);
		y = std::min(y, that.y);
		z = std::min(z, that.z);
	}

	inline void max(const v3& that) {
		x = std::max(x, that.x);
		y = std::max(y, that.y);
		z = std::max(z, that.z);
	}

	inline void set(float* v) {
		x = v[0], y = v[1], z = v[2];
	}
	
	inline void set(float v) {
		x = y = z = v;
	}

	inline void set(float x_, float y_, float z_) {
		x = x_, y = y_, z = z_;
	}

	static v3 sub(const v3& a, const v3& b) {
		return v3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	static float dot2d(const v3& a, const v3& b) {
		return a.x * b.x + a.y * b.y;
	}

	inline float dot2d(const v3& that) const {
		return (x * that.x) + (y * that.y);
	}
};