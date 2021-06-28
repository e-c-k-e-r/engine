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

		pod::Vector3t<T> position = {0, 0,0 };
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
		template<typename T> pod::Transform<T> /*UF_API*/ reorient( const pod::Transform<T>& transform );
		template<typename T> pod::Transform<T>& /*UF_API*/ rotate( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta );
		template<typename T> pod::Transform<T>& /*UF_API*/ rotate( pod::Transform<T>& transform, const pod::Quaternion<T>& quat );
		template<typename T> pod::Transform<T>& /*UF_API*/ scale( pod::Transform<T>& transform, const pod::Vector3t<T>& factor );
		template<typename T> inline pod::Transform<T> /*UF_API*/ condense( const pod::Transform<T>& transform );
		template<typename T> pod::Transform<T> /*UF_API*/ flatten( const pod::Transform<T>& transform, size_t depth = SIZE_MAX );
		template<typename T> pod::Matrix4t<T> /*UF_API*/ model( const pod::Transform<T>& transform, bool flatten = false, size_t depth = SIZE_MAX );
		template<typename T> pod::Transform<T> /*UF_API*/ fromMatrix( const pod::Matrix4t<T>& matrix );
		template<typename T> pod::Transform<T>& /*UF_API*/ reference( pod::Transform<T>& transform, const pod::Transform<T>& parent, bool reorient = true );
		
		template<typename T> std::string /*UF_API*/ toString( const pod::Transform<T>&, bool flatten = true );
		template<typename T> ext::json::Value /*UF_API*/ encode( const pod::Transform<T>&, bool flatten = true, const ext::json::EncodingSettings& = {} );
		template<typename T> pod::Transform<T>& /*UF_API*/ decode( const ext::json::Value&, pod::Transform<T>& );
		template<typename T> pod::Transform<T> /*UF_API*/ decode( const ext::json::Value&, const pod::Transform<T>& = {} );
	}
}

#include <sstream>
namespace uf {
	namespace string {
		template<typename T>
		std::string toString( const pod::Transform<T>& v, bool flatten = true );
	}
}
namespace ext {
	namespace json {
		template<typename T>
		ext::json::Value encode( const pod::Transform<T>& v, bool flatten = false );
		template<typename T>
		pod::Transform<T> decode( const ext::json::Value& v );
	}
}

#include "transform/transform.inl"