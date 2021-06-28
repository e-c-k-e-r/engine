// 	Equality checking
// 	Equality check between two quaternions (less than)
template<typename T> size_t uf::quaternion::compareTo( const T& left, const T& right ) {
	return uf::vector::compareTo(left, right);
//	return uf::quaternion::angle(left) > uf::quaternion::angle(right);
}
// 	Equality check between two quaternions (equals)
template<typename T> bool uf::quaternion::equals( const T& left, const T& right ) {
	return uf::quaternion::compareTo( left, right ) == 0;
}
// 	Basic arithmetic
// 	Multiplies two quaternions of same type and size together
template<typename T> T uf::quaternion::multiply( const T& left, const T& right ) {
	T q1 = uf::quaternion::normalize(left);
	T q2 = uf::quaternion::normalize(right);
	T q;
	
	q.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	q.y = q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z;
	q.z = q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x;
	q.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;

	return uf::quaternion::normalize( q );
}
// 	Multiplies this quaternion by a scalar
/*
template<typename T> T uf::quaternion::multiply( const T& quaternion, const typename T::type_t& scalar ) {
	return uf::vector::multiply( quaternion, scalar );
}
*/
// 	Flip sign of all components
template<typename T> T uf::quaternion::negate( const T& quaternion ) {
	return uf::quaternion::inverse(quaternion);
}
template<typename T> pod::Quaternion<T> uf::quaternion::identity() {
	return pod::Quaternion<T>{ 0, 0, 0, 1 };
}
// 	Writes to first value
// 	Multiplies two quaternions of same type and size together
template<typename T> T& uf::quaternion::multiply( T& left, const T& right ) {
	return left = uf::quaternion::multiply((const T&)  left, right );
}
// 	Multiplies a quaternion and a vector of same type and size together
template<typename T> pod::Vector3t<T> uf::quaternion::rotate( const pod::Quaternion<T>& left, const pod::Vector3t<T>& right ) {
	pod::Vector3t<T> qVec = { left.x, left.y, left.z };
	const T s = left.w;
	return uf::vector::multiply( qVec, static_cast<T>(2) * uf::vector::dot( qVec, right ) ) + uf::vector::multiply( right, s*s - uf::vector::dot( qVec, qVec )) +  ( uf::vector::cross( qVec, right ) * static_cast<T>(2) * s );
}
template<typename T> pod::Vector4t<T> uf::quaternion::rotate( const pod::Quaternion<T>& left, const pod::Vector4t<T>& right ) {
	pod::Vector3t<T> vector = uf::quaternion::rotate( left, {right.x, right.y, right.z} );
	return {vector.x, vector.y, vector.z, left.w};
}
// 	Flip sign of all components
template<typename T> T& uf::quaternion::negate( T& quaternion ) {
	return quaternion = uf::quaternion::negate((const T&) quaternion);
}
// 	Normalizes a quaternion
template<typename T> T& uf::quaternion::normalize( T& quaternion ) {
	return quaternion = uf::quaternion::normalize((const T&) quaternion);
}
// 	Complex arithmetic
// 	Compute the dot product between two quaternions
template<typename T> typename T::type_t uf::quaternion::dot( const T& left, const T& right ) {
	return uf::vector::dot(left, right);
}
// 	Compute the angle between two quaternions
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
	T const y = static_cast<T>(2) * (q.y * q.z + q.w * q.x);
#if UF_USE_SIMD
	uf::simd::value Q = q;
	pod::Quaternion<T> s = uf::simd::mul( Q, Q );
	T const x = s.w - s.x - s.y - s.z;
#else
	T const x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;
#endif

	T epsilon = std::numeric_limits<T>::epsilon();
	if ( fabs(x) < epsilon && fabs(y) < epsilon  ) //avoid atan2(0,0) - handle singularity - Matiis
		return static_cast<T>(static_cast<T>(2) * atan2(q.x, q.w));

	return static_cast<T>(atan2(y, x));
}
template<typename T> T uf::quaternion::yaw( const pod::Quaternion<T>& q ) {
	return asin(std::clamp(static_cast<T>(-2) * (q.x * q.z - q.w * q.y), static_cast<T>(-1), static_cast<T>(1)));
}
template<typename T> T uf::quaternion::roll( const pod::Quaternion<T>& q ) {
	T const y = static_cast<T>(2) * (q.x * q.y + q.w * q.z);
#if UF_USE_SIMD
	uf::simd::value Q = q;
	pod::Quaternion<T> s = uf::simd::mul( Q, Q );
	T const x = s.w - s.x - s.y - s.z;
#else
	T const x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;
#endif

	T epsilon = std::numeric_limits<T>::epsilon();
	if ( fabs(x) < epsilon && fabs(y) < epsilon  ) //avoid atan2(0,0) - handle singularity - Matiis
		return static_cast<T>(0);

	return static_cast<T>(atan2(y, x));
}

// 	Linearly interpolate between two quaternions
template<typename T> T uf::quaternion::lerp( const T& from, const T& to, typename T::type_t delta ) {
	return uf::vector::lerp( from, to, delta );
}
// 	Spherically interpolate between two quaternions
template<typename T> T uf::quaternion::slerp( const T& x, const T& y, typename T::type_t a ) {
	T z = y;
	auto cosTheta = uf::quaternion::dot( x, y );
	if( cosTheta < 0 ) {
		z = -y;
		cosTheta = -cosTheta;
	}
	if( cosTheta > 1 - std::numeric_limits<typename T::type_t>::epsilon() ) return uf::vector::mix( x, z, a );
	typename T::type_t angle = acos(cosTheta);
	return uf::vector::divide( uf::vector::add(uf::vector::multiply(x, sin(static_cast<typename T::type_t>(1) - a * angle)), uf::vector::multiply(z, sin(a * angle))), sin(angle) );
//	return (x * sin((static_cast<typename T::type_t>(1) - a) * angle) + z * sin(a * angle)) / sin(angle);
}

// 	Compute the distance between two quaternions (doesn't sqrt)
template<typename T> typename T::type_t uf::quaternion::distanceSquared( const T& a, const T& b ) {
	return uf::vector::distanceSquared(a, b);
}
// 	Compute the distance between two quaternions
template<typename T> typename T::type_t uf::quaternion::distance( const T& a, const T& b ) {
	return uf::vector::distance(a, b);
}
// 	Gets the magnitude of the quaternion
template<typename T> typename T::type_t uf::quaternion::magnitude( const T& quaternion ) {
	return uf::vector::magnitude(quaternion);
}
// 	Compute the norm of the quaternion
template<typename T> typename T::type_t uf::quaternion::norm( const T& quaternion ) {
	return uf::vector::norm(quaternion);
}
// 	Normalizes a quaternion
template<typename T> T uf::quaternion::normalize( const T& quaternion ) {
	return uf::vector::normalize(quaternion);
}

// Quaternion ops
template<typename T> pod::Matrix4t<typename T::type_t> uf::quaternion::matrix( const T& q ) {
	T normal = uf::quaternion::normalize( q );

	const typename T::type_t xx = 2 * normal.x * normal.x;
	const typename T::type_t xy = 2 * normal.x * normal.y;
	const typename T::type_t xz = 2 * normal.x * normal.z;
	const typename T::type_t xw = 2 * normal.x * -normal.w;

	const typename T::type_t yy = 2 * normal.y * normal.y;
	const typename T::type_t yz = 2 * normal.y * normal.z;
	const typename T::type_t yw = 2 * normal.y * -normal.w;

	const typename T::type_t zz = 2 * normal.z * normal.z;
	const typename T::type_t zw = 2 * normal.z * -normal.w;

//	const typename T::type_t ww = w * w;

	return pod::Matrix4t<typename T::type_t>({
	1 - yy - zz, 			xy - zw, 			xz + yw, 	0,
		xy + zw, 		1 - xx - zz, 			yz - xw, 	0,
		xz - yw, 			yz + xw, 		1 - xx - yy, 	0,
		0, 0, 0, 1
	});
}
template<typename T> pod::Quaternion<T> uf::quaternion::axisAngle( const pod::Vector3t<T>& axis, T angle ) { 
	pod::Quaternion<T> q;

	T sinAngle = sin( angle * static_cast<T>(0.5) );
	T cosAngle = cos( angle * static_cast<T>(0.5) );
#if UF_USE_SIMD
	q = uf::simd::mul( uf::simd::value(axis.x, axis.y, axis.z, static_cast<T>(1) ), uf::simd::value(sinAngle, sinAngle, sinAngle, cosAngle) );
#else
	q.x = axis.x * sinAngle;
	q.y = axis.y * sinAngle;
	q.z = axis.z * sinAngle;
	q.w = cosAngle;
#endif
	uf::quaternion::normalize(q);
	return q;
}
template<typename T> pod::Quaternion<T> uf::quaternion::unitVectors( const pod::Vector3t<T>& u, const pod::Vector3t<T>& v ) {
	T dot = uf::vector::dot(u, v);
	static const T EPSILON = static_cast<T>(0.00001);
	if ( dot + 1 < EPSILON ) return uf::quaternion::axisAngle( uf::vector::normalize(u), static_cast<T>(3.1415926) );
	T mag = sqrt( static_cast<T>(2) + static_cast<T>(2) * dot );
	pod::Vector3t<T> w = uf::vector::multiply(uf::vector::cross(u, v), (static_cast<T>(1) / mag));
	return {
		.x = w.x,
		.y = w.y,
		.z = w.z,
		.w = mag * static_cast<T>(0.5)
	};
}
template<typename T> pod::Quaternion<T> uf::quaternion::lookAt( const pod::Vector3t<T>& at, const pod::Vector3t<T>& _up ) { 
	pod::Vector3t<T> up = _up;
	pod::Vector3t<T> forward = uf::vector::normalize( at ) ;
	uf::vector::orthonormalize( up, forward );
	pod::Vector3t<T> right = uf::vector::cross( up, forward );
	pod::Quaternion<T> q;
#if UF_USE_SIMD
	T w = sqrtf(static_cast<T>(1) + right.x + up.y + forward.z) * static_cast<T>(0.5);
	float w4_recip = static_cast<T>(1) / (static_cast<T>(4) * w);
	q = uf::simd::mul( uf::simd::sub( uf::simd::value( forward.y, right.z, up.x, static_cast<T>(0) ), uf::simd::value( up.z, forward.x, right.y, static_cast<T>(0) ) ), w4_recip );
	q.w = w;
#else
	q.w = sqrtf(static_cast<T>(1) + right.x + up.y + forward.z) * static_cast<T>(0.5);
	float w4_recip = static_cast<T>(1) / (static_cast<T>(4) * q.w);
	q.x = (forward.y - up.z) * w4_recip;
	q.y = (right.z - forward.x) * w4_recip;
	q.z = (up.x - right.y) * w4_recip;
#endif
	return uf::quaternion::inverse( uf::quaternion::normalize( q ) );
//	return q;
}

template<typename T> T& uf::quaternion::conjugate( T& q ) {
#if UF_USE_SIMD
	return q = uf::simd::mul( q, static_cast<typename T::type_t>(-1) );
#endif
	return q = {
		.x = -q.x,	
		.y = -q.y,	
		.z = -q.z,	
		.w =  q.w	
	};
}
template<typename T> T uf::quaternion::conjugate( const T& q ) {
#if UF_USE_SIMD
	return uf::simd::mul( q, static_cast<typename T::type_t>(-1) );
#endif
	return {
		.x = -q.x,	
		.y = -q.y,	
		.z = -q.z,	
		.w =  q.w	
	};
}
template<typename T> T& uf::quaternion::inverse( T& q ) {
#if UF_USE_SIMD
	uf::simd::value Q = q;
	return q = uf::simd::div( uf::simd::mul( Q, { static_cast<typename T::type_t>(-1), static_cast<typename T::type_t>(-1), static_cast<typename T::type_t>(-1), static_cast<typename T::type_t>(1) } ), uf::simd::dot( Q, Q ) );
#endif
	return q = uf::quaternion::conjugate( (const T&) q ) / uf::quaternion::dot( q, q );
}
template<typename T> T uf::quaternion::inverse( const T& q ) {
#if UF_USE_SIMD
	uf::simd::value Q = q;
	return uf::simd::div( uf::simd::mul( Q, { static_cast<typename T::type_t>(-1), static_cast<typename T::type_t>(-1), static_cast<typename T::type_t>(-1), static_cast<typename T::type_t>(1) } ), uf::simd::dot( Q, Q ) );
#endif
	return uf::quaternion::conjugate( q ) / uf::quaternion::dot( q, q );
}
template<typename T> pod::Quaternion<T> uf::quaternion::fromMatrix( const pod::Matrix4t<T>& m ) {
	pod::Quaternion<T> q;
	T m0  = m[(4*0)+0];
	T m5  = m[(4*1)+1];
	T m10 = m[(4*2)+2];

#if UF_USE_SIMD
	q = uf::simd::div( uf::simd::sqrt( uf::simd::max( static_cast<T>(0), uf::simd::add( uf::simd::add( uf::simd::add( static_cast<T>(1), uf::simd::value( m0, m0, -m0, -m0 ) ), uf::simd::value( m5, -m5, -m5, -m5 ) ), { m10, -m10, -m10, m10 } ) ) ), 2.0f );
	pod::Vector4f signs = uf::simd::sub( uf::simd::value( m[(4*1)+2], m[(4*2)+0], m[(4*0)+1], static_cast<T>(0) ), uf::simd::value( m[(4*2)+1], m[(4*0)+2], m[(4*1)+0], 0.0f ) );
	return {
		copysign( q.x, signs.x ),
		copysign( q.y, signs.y ),
		copysign( q.z, signs.z ),
		q.w
	};
#else
	q.w = sqrt(fmax(0, 1 + m0 + m5 + m10)) * static_cast<T>(0.5);
	q.x = sqrt(fmax(0, 1 + m0 - m5 - m10)) * static_cast<T>(0.5);
	q.y = sqrt(fmax(0, 1 - m0 + m5 - m10)) * static_cast<T>(0.5);
	q.z = sqrt(fmax(0, 1 - m0 - m5 + m10)) * static_cast<T>(0.5);

	q.x = copysign(q.x, m[(4*1)+2] - m[(4*2)+1]);
	q.y = copysign(q.y, m[(4*2)+0] - m[(4*0)+2]);
	q.z = copysign(q.z, m[(4*0)+1] - m[(4*1)+0]);
#endif
	return q;
}