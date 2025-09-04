#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>

namespace pod {
	struct Plane {
		alignas(16) pod::Vector3f normal;
		float offset;
	};

	struct AABB {
		alignas(16) pod::Vector3f min;
		alignas(16) pod::Vector3f max;
	};

	struct Sphere {
		float radius;
	};

	struct Capsule {
		float radius;
		float halfHeight;
	};

	struct Ray {
		alignas(16) pod::Vector3f origin;
		alignas(16) pod::Vector3f direction;
	};

	struct Triangle {
		alignas(16) pod::Vector3f points[3];
	};

	struct TriangleWithNormal : Triangle {
		alignas(16) pod::Vector3f normal;
	};
	struct TriangleWithNormals : Triangle {
		alignas(16) pod::Vector3f normals[3];
	};

	template<typename T>
	struct Polygon {
		uf::stl::vector<T> points;
	};
}

