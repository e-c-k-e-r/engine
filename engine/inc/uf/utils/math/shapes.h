#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>

namespace pod {
	struct Plane {
		pod::Vector3f normal;
		float offset;
	};

	struct AABB {
		pod::Vector3f min;
		pod::Vector3f max;
	};

	struct Sphere {
		float radius;
	};

	struct Capsule {
		float radius;
		float halfHeight;
	};

	struct Ray {
		pod::Vector3f origin;
		pod::Vector3f direction;
	};

	struct Triangle {
		pod::Vector3f points[3];
	};

	struct TriangleWithNormal : Triangle {
		pod::Vector3f normal;
	};
	struct TriangleWithNormals : Triangle {
		pod::Vector3f normals[3];
	};

	template<typename T>
	struct Polygon {
		uf::stl::vector<T> points;
	};
}

