namespace pod {
	// Simple quaterions (designed [to store in arrays] with minimal headaches)
	template<typename T = NUM> using Quaternion = Vector4t<T>;
}

// snip header

template<typename T> pod::Quaternion<T> uf::quaternion::identity() {
	return pod::Quaternion<T>{ 0, 0, 0, 1 };
}
template<typename T> T uf::quaternion::multiply( const T& q1, const T& q2 ) {
#if UF_USE_SIMD
	if constexpr (std::is_same_v<typename T::type_t, float>) {
		return uf::simd::quatMul( q1 , q2 );
	}
#endif
	return {
		q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
		q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z,
		q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x,
		q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
	};
}
template<typename T> pod::Vector3t<T> uf::quaternion::rotate( const pod::Quaternion<T>& Q, const pod::Vector3t<T>& v ) {
#if UF_USE_SIMD
	if constexpr (std::is_same_v<T,float>) {
		return uf::simd::quatRot_3f( Q, v );
	}
#endif
	pod::Vector3t<T> q = { Q.x, Q.y, Q.z };
	const T s = Q.w;
	return uf::vector::multiply(q, static_cast<T>(2) * uf::vector::dot(q, v)) + uf::vector::multiply(v, s*s - uf::vector::dot(q, q)) + (uf::vector::cross(q, v) * static_cast<T>(2) * s);
}
template<typename T> pod::Vector4t<T> uf::quaternion::rotate( const pod::Quaternion<T>& q, const pod::Vector4t<T>& v ) {
	pod::Vector3t<T> vector = uf::quaternion::rotate(q, { v.x, v.y, v.z });
	return { vector.x, vector.y, vector.z, v.w };
}
template<typename T> typename T::type_t uf::quaternion::dot( const T& left, const T& right ) {
	return uf::vector::dot(left, right);
}
template<typename T> pod::Angle uf::quaternion::angle( const T& a, const T& b ) {
	T tmp = b * uf::quaternion::inverse(a);
	return acosf(tmp.w) * static_cast<typename T::type_t>(2);
}
template<typename T> pod::Vector3t<T> uf::quaternion::eulerAngles( const pod::Quaternion<T>& quaternion ) {
	return pod::Vector3t<T>{
		uf::quaternion::pitch( quaternion ),
		uf::quaternion::yaw( quaternion ),
		uf::quaternion::roll( quaternion ),
	};
}
template<typename T> T uf::quaternion::pitch( const pod::Quaternion<T>& q ) {
	const T y = static_cast<T>(2) * (q.y * q.z + q.w * q.x);
	auto s = uf::vector::multiply( q, q );
	const T x = s.w - s.x - s.y - s.z;
	T epsilon = std::numeric_limits<T>::epsilon();
	if ( fabs(x) < epsilon && fabs(y) < epsilon  ) return static_cast<T>(static_cast<T>(2) * atan2(q.x, q.w));
	return static_cast<T>(atan2(y, x));
}
template<typename T> T uf::quaternion::yaw( const pod::Quaternion<T>& q ) {
	return asin(std::clamp(static_cast<T>(-2) * (q.x * q.z - q.w * q.y), static_cast<T>(-1), static_cast<T>(1)));
}
template<typename T> T uf::quaternion::roll( const pod::Quaternion<T>& q ) {
	const T y = static_cast<T>(2) * (q.x * q.y + q.w * q.z);
	auto s = uf::vector::multiply( q, q );
	const T x = s.w - s.x - s.y - s.z;
	T epsilon = std::numeric_limits<T>::epsilon();
	if ( fabs(x) < epsilon && fabs(y) < epsilon ) return static_cast<T>(0);
	return static_cast<T>(atan2(y, x));
}
template<typename T> T uf::quaternion::lerp( const T& from, const T& to, typename T::type_t delta ) {
	return uf::vector::lerp( from, to, delta );
}
template<typename T> T uf::quaternion::slerp( const T& x, const T& y, typename T::type_t a ) {
	T z = y;
	auto cosTheta = uf::quaternion::dot(x, y);
	if ( cosTheta < 0 ) {
		z = -y;
		cosTheta = -cosTheta;
	}
	if (cosTheta > 1 - std::numeric_limits<typename T::type_t>::epsilon()) return uf::vector::mix(x, z, a);

	typename T::type_t angle = acos(cosTheta);
	// return ( sin( ( 1 - a ) * angle) * x + sin( a * angle ) * y ) / sin( angle );
	return uf::vector::divide( uf::vector::add( uf::vector::multiply(x, sin((1 - a) * angle)), uf::vector::multiply(z, sin(a * angle)) ), sin( angle ) );
}
template<typename T> typename T::type_t uf::quaternion::distanceSquared( const T& a, const T& b ) {
	return uf::vector::distanceSquared(a, b);
}
template<typename T> typename T::type_t uf::quaternion::distance( const T& a, const T& b ) {
	return uf::vector::distance(a, b);
}
template<typename T> typename T::type_t uf::quaternion::magnitude( const T& quaternion ) {
	return uf::vector::magnitude(quaternion);
}
template<typename T> typename T::type_t uf::quaternion::norm( const T& quaternion ) {
	return uf::vector::norm(quaternion);
}
template<typename T> T uf::quaternion::normalize( const T& quaternion ) {
	return uf::vector::normalize(quaternion);
}
template<typename T> pod::Matrix4t<T> uf::quaternion::matrix( const pod::Quaternion<T>& q ) {
#if UF_USE_SIMD
	if constexpr ( std::is_same_v<T,float> ) {
		return uf::simd::quatMat( q );
	}
#endif
	auto normal = uf::quaternion::normalize(q);

	const T xx = 2 * normal.x * normal.x;
	const T xy = 2 * normal.x * normal.y;
	const T xz = 2 * normal.x * normal.z;
	const T xw = 2 * normal.x * normal.w;

	const T yy = 2 * normal.y * normal.y;
	const T yz = 2 * normal.y * normal.z;
	const T yw = 2 * normal.y * normal.w;

	const T zz = 2 * normal.z * normal.z;
	const T zw = 2 * normal.z * normal.w;

	return pod::Matrix4t<T>({
		1 - yy - zz,   xy + zw,       xz - yw,   0,
		xy - zw,       1 - xx - zz,   yz + xw,   0,
		xz + yw,       yz - xw,       1 - xx - yy, 0,
		0,             0,             0,         1
	});
}
template<typename T> pod::Quaternion<T> uf::quaternion::axisAngle( const pod::Vector3t<T>& axis, T angle ) { 
	pod::Quaternion<T> q;

	T sinAngle = sin( angle * static_cast<T>(0.5) );
	T cosAngle = cos( angle * static_cast<T>(0.5) );

	q = pod::Vector4t<T>{ axis.x, axis.y, axis.z, 1 } * pod::Vector4t<T>{ sinAngle, sinAngle, sinAngle, cosAngle };
	return uf::quaternion::normalize( q );
}
template<typename T> pod::Quaternion<T> uf::quaternion::unitVectors( const pod::Vector3t<T>& u, const pod::Vector3t<T>& v ) {
	static const T EPSILON = static_cast<T>(1e-6);

	pod::Vector3t<T> uNorm = uf::vector::normalize( u );
	pod::Vector3t<T> vNorm = uf::vector::normalize( v );

	T dot = uf::vector::dot( uNorm, vNorm );

	if ( dot < -1 + EPSILON ) {
		pod::Vector3t<T> orthogonal = (fabs(uNorm.x) > fabs(uNorm.z)) ? pod::Vector3t<T>{ -uNorm.y, uNorm.x, 0 } : pod::Vector3t<T>{ 0, -uNorm.z, uNorm.y };
		orthogonal = uf::vector::normalize( orthogonal );
		return uf::quaternion::axisAngle( orthogonal, static_cast<T>(M_PI) );
	}

	pod::Vector3t<T> cross = uf::vector::cross(uNorm, vNorm);
	T s = sqrt((1 + dot) * 2);

	return uf::quaternion::normalize({
		.x = cross.x / s,
		.y = cross.y / s,
		.z = cross.z / s,
		.w = s * static_cast<T>(0.5)
	});
}
template<typename T> pod::Quaternion<T> uf::quaternion::lookAt( const pod::Vector3t<T>& at, const pod::Vector3t<T>& _up ) { 
	pod::Vector3t<T> forward = uf::vector::normalize(at);
	pod::Vector3t<T> up = uf::vector::orthonormalize( _up, forward );
	pod::Vector3t<T> right = uf::vector::cross(up, forward);
	pod::Matrix4t<T> m({
		right.x,   up.x,   forward.x,   0,
		right.y,   up.y,   forward.y,   0,
		right.z,   up.z,   forward.z,   0,
		0,         0,      0,           1
	});
	return uf::quaternion::normalize( uf::quaternion::fromMatrix( m ) );
}


template<typename T> T uf::quaternion::conjugate( const T& q ) {
	return uf::vector::multiply( q, { -1, -1, -1, 1 } );
}

template<typename T> T uf::quaternion::inverse( const T& q ) {
	return uf::quaternion::conjugate( q ) / uf::quaternion::dot( q, q );
}

template<typename T> T& uf::quaternion::multiply_( T& left, const T& right ) {
	return left = uf::quaternion::multiply((const T&) left, right );
}
template<typename T> T& uf::quaternion::normalize_( T& q ) {
	return q = uf::quaternion::normalize((const T&) q);
}
template<typename T> T& uf::quaternion::conjugate_( T& q ) {
	return q = uf::quaternion::conjugate((const T&) q);
}
template<typename T> T& uf::quaternion::inverse_( T& q ) {
	return q = uf::quaternion::inverse((const T&) q);
}

template<typename T> pod::Quaternion<T> uf::quaternion::fromMatrix( const pod::Matrix4t<T>& m ) {
	pod::Quaternion<T> q;

	T m00 = m[0],  m01 = m[1],  m02 = m[2];
	T m10 = m[4],  m11 = m[5],  m12 = m[6];
	T m20 = m[8],  m21 = m[9],  m22 = m[10];

	T trace = m00 + m11 + m22;
	if ( trace > 0 ) {
		T s = sqrt(trace + 1) * static_cast<T>(2);
		q.w = static_cast<T>(0.25) * s;
		q.x = (m21 - m12) / s;
		q.y = (m02 - m20) / s;
		q.z = (m10 - m01) / s;
	}
	else if ( m00 > m11 && m00 > m22 ) {
		T s = sqrt(1 + m00 - m11 - m22) * static_cast<T>(2);
		q.w = (m21 - m12) / s;
		q.x = static_cast<T>(0.25) * s;
		q.y = (m01 + m10) / s;
		q.z = (m02 + m20) / s;
	}
	else if ( m11 > m22 ) {
		T s = sqrt(1 + m11 - m00 - m22) * static_cast<T>(2);
		q.w = (m02 - m20) / s;
		q.x = (m01 + m10) / s;
		q.y = static_cast<T>(0.25) * s;
		q.z = (m12 + m21) / s;
	}
	else {
		T s = sqrt(1 + m22 - m00 - m11) * static_cast<T>(2);
		q.w = (m10 - m01) / s;
		q.x = (m02 + m20) / s;
		q.y = (m12 + m21) / s;
		q.z = static_cast<T>(0.25) * s;
	}

	return uf::quaternion::normalize(q);
}