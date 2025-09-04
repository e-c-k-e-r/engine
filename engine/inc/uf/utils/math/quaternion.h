#pragma once

#include <uf/config.h>

#include <sstream>
#include <uf/utils/memory/vector.h>
#include <cmath>
#include <stdint.h>

#include <uf/utils/math/angle.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>

#include "math.h"
namespace pod {
	// Simple quaterions (designed [to store in arrays] with minimal headaches)
	template<typename T = NUM> using Quaternion = Vector4t<T>;
}

namespace uf {
	namespace quaternion {
		template<typename T = NUM> pod::Quaternion<T> /*UF_API*/ identity();
		template<typename T> T /*UF_API*/ multiply( const T& left, const T& right );
		template<typename T> pod::Vector3t<T> /*UF_API*/ rotate( const pod::Quaternion<T>& left, const pod::Vector3t<T>& right );
		template<typename T> pod::Vector4t<T> /*UF_API*/ rotate( const pod::Quaternion<T>& left, const pod::Vector4t<T>& right );
		template<typename T> typename T::type_t /*UF_API*/ sum( const T& vector );
		template<typename T> typename T::type_t /*UF_API*/ product( const T& vector );


		template<typename T> typename T::type_t /*UF_API*/ dot( const T& left, const T& right );
		template<typename T> pod::Angle /*UF_API*/ angle( const T& a, const T& b );

		template<typename T> T /*UF_API*/ lerp( const T& from, const T& to, typename T::type_t delta );
		template<typename T> T /*UF_API*/ slerp( const T& from, const T& to, typename T::type_t delta );

		template<typename T> typename T::type_t /*UF_API*/ distanceSquared( const T& a, const T& b );
		template<typename T> typename T::type_t /*UF_API*/ distance( const T& a, const T& b );
		template<typename T> typename T::type_t /*UF_API*/ magnitude( const T& vector );
		template<typename T> typename T::type_t /*UF_API*/ norm( const T& vector );
		template<typename T> T /*UF_API*/ normalize( const T& vector );

		template<typename T> pod::Matrix4t<T> matrix( const pod::Quaternion<T>& quaternion );
		template<typename T> pod::Quaternion<T> axisAngle( const pod::Vector3t<T>& axis, T angle );
		template<typename T> pod::Quaternion<T> unitVectors( const pod::Vector3t<T>& u, const pod::Vector3t<T>& v );
		template<typename T> pod::Quaternion<T> lookAt( const pod::Vector3t<T>& source, const pod::Vector3t<T>& destination );
		
		template<typename T> T conjugate( const T& quaternion );
		template<typename T> T inverse( const T& quaternion );

		template<typename T> T& /*UF_API*/ multiply_( T& left, const T& right );
		template<typename T> T& /*UF_API*/ normalize_( T& vector );
		template<typename T> T& conjugate_( T& quaternion );
		template<typename T> T& inverse_( T& quaternion );

		template<typename T> pod::Vector3t<T> eulerAngles( const pod::Quaternion<T>& quaternion );
		template<typename T> T pitch( const pod::Quaternion<T>& quaternion );
		template<typename T> T yaw( const pod::Quaternion<T>& quaternion );
		template<typename T> T roll( const pod::Quaternion<T>& quaternion );
		
		template<typename T> pod::Quaternion<T> fromMatrix( const pod::Matrix4t<T>& matrix );
	}
}

#include "quaternion/quaternion.inl"