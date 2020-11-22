#pragma once

#include <uf/config.h>

#include <sstream>
#include <vector>
#include <cmath>
#include <stdint.h>

#include <uf/utils/math/angle.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#ifdef UF_USE_GLM
	#include "glm.h"
#endif
#include "math.h"
namespace pod {
	// Simple transforms (designed [to store in arrays] with minimal headaches)
	template<typename T = pod::Math::num_t>
	struct /*UF_API*/ Transform {
		typedef T type_t;

		pod::Vector3t<T> position;
		pod::Vector3t<T> scale = {1, 1, 1};

		pod::Vector3t<T> up = {0, 1, 0};
		pod::Vector3t<T> right = {1, 0, 0};
		pod::Vector3t<T> forward = {0, 0, 1};
		pod::Quaternion<T> orientation = {0, 0, 0, 1};

		pod::Matrix4t<T> model = uf::matrix::identity();

		pod::Transform<T>* reference = NULL;
	};
}

namespace uf {
	namespace transform {
		template<typename T> pod::Transform<T>& /*UF_API*/ initialize( pod::Transform<T>& transform );
		template<typename T> pod::Transform<T> /*UF_API*/ initialize();
		template<typename T> pod::Transform<T>& /*UF_API*/ lookAt( pod::Transform<T>& transform, const pod::Vector3t<T>& at );
		template<typename T> pod::Transform<T>& /*UF_API*/ move( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta );
		template<typename T> pod::Transform<T>& /*UF_API*/ move( pod::Transform<T>& transform, const pod::Vector3t<T>& delta );
		template<typename T> pod::Transform<T>& /*UF_API*/ reorient( pod::Transform<T>& transform );
		template<typename T> pod::Transform<T>& /*UF_API*/ rotate( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta );
		template<typename T> pod::Transform<T>& /*UF_API*/ rotate( pod::Transform<T>& transform, const pod::Quaternion<T>& quat );
		template<typename T> pod::Transform<T>& /*UF_API*/ scale( pod::Transform<T>& transform, const pod::Vector3t<T>& factor );
		template<typename T> pod::Transform<T> /*UF_API*/ flatten( const pod::Transform<T>& transform, bool invert = false);
		template<typename T> pod::Matrix4t<T> /*UF_API*/ model( const pod::Transform<T>& transform, bool flatten = true );
		template<typename T> pod::Transform<T> fromMatrix( const pod::Matrix4t<T>& matrix );
	}
}

#include "transform/transform.inl"