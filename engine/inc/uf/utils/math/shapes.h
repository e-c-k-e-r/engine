#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>

#define OBB_EXTENT_CENTER 0

namespace pod {
	struct Plane {
		pod::Vector3f normal;
		float offset;
	};

	struct AABB {
		pod::Vector3f min;
		pod::Vector3f max;
	};

	struct OBB {
	#if OBB_EXTENT_CENTER
		pod::Vector3f extent;
		pod::Vector3f center;
	#else
		pod::Vector3f center;
		pod::Vector3f extent;
	#endif
	};

	struct Sphere {
		float radius;
	};

	struct Capsule {
		float radius;
		pod::Vector3f up;
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

