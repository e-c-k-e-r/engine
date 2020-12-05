// 	Equality checking
// 	Equality check between two quaternions (less than)
template<typename T> std::size_t uf::quaternion::compareTo( const T& left, const T& right ) {
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
	
//	if ( uf::vector::equals(q1,{0,0,0,0}) ) q1 = {0,0,0,1};
//	if ( uf::vector::equals(q2,{0,0,0,0}) ) q2 = {0,0,0,1};

/*
	q.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	q.y = q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z;
	q.z = q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x;
	q.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
*/
	q.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	q.y = q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z;
	q.z = q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x;
	q.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;

	uf::quaternion::normalize( q );
	return q;
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
	T s = left.w;
	return uf::vector::multiply( qVec, 2 * uf::vector::dot( qVec, right ) ) + uf::vector::multiply( right, s*s - uf::vector::dot( qVec, qVec )) +  ( uf::vector::cross( qVec, right ) * 2 * s );
/*
	pod::Vector3t<T> quatVector = {left.x, left.y, left.z};
	pod::Vector3t<T> uv = uf::vector::cross( quatVector, right );
	pod::Vector3t<T> uuv = uf::vector::cross( quatVector, uv );
	return right + ((uv * left.w) + uuv) * 2;
*/
/*
	pod::Vector3t<T> normalized = uf::vector::normalize(right);
	pod::Vector4t<T> rotated = {normalized.x, normalized.y, normalized.z, 1.0};
	pod::Matrix4t<T> rotation = uf::quaternion::matrix(left);
	pod::Vector4t<T> product = uf::matrix::multiply<T>(rotation, rotated);
	return { product.x, product.y, product.z };
*/
}
template<typename T> pod::Vector4t<T> uf::quaternion::rotate( const pod::Quaternion<T>& left, const pod::Vector4t<T>& right ) {
	pod::Vector3t<T> vector = uf::quaternion::rotate( left, {right.x, right.y, right.z} );
	return {vector.x, vector.y, vector.z, left.w};
/*
	pod::Vector3t<T> normalized = uf::vector::normalize(right);
	pod::Vector4t<T> rotated = {normalized.x, normalized.y, normalized.z, normalized.w};
	pod::Matrix4t<T> rotation = uf::quaternion::matrix(left);
	pod::Vector4t<T> product = uf::matrix::multiply<T>(rotation, rotated);
	return { product.x, product.y, product.z, product.w };
*/
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
	return acosf(tmp.w) * 2.0;
}
template<typename T> pod::Vector3t<T> uf::quaternion::eulerAngles( const pod::Quaternion<T>& quaternion ) {
	return pod::Vector3t<T>{
		uf::quaternion::pitch( quaternion ),
		uf::quaternion::yaw( quaternion ),
		uf::quaternion::roll( quaternion ),
	};
}
template<typename T> T uf::quaternion::pitch( const pod::Quaternion<T>& q ) {
	//return T(atan(T(2) * (q.y * q.z + q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z));
	T const y = static_cast<T>(2) * (q.y * q.z + q.w * q.x);
	T const x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;

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
	T const x = q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z;

	T epsilon = std::numeric_limits<T>::epsilon();
	if ( fabs(x) < epsilon && fabs(y) < epsilon  ) //avoid atan2(0,0) - handle singularity - Matiis
		return static_cast<T>(0);

	return static_cast<T>(atan2(y, x));
}

// 	Linearly interpolate between two quaternions
template<typename T> T uf::quaternion::lerp( const T& from, const T& to, double delta ) {
	return uf::vector::lerp( from, to, delta );
}
// 	Spherically interpolate between two quaternions
template<typename T> T uf::quaternion::slerp( const T& x, const T& y, double a ) {
//	return uf::vector::slerp( from, to, delta );
	T z = y;

	auto cosTheta = uf::quaternion::dot( x, y );

	// If cosTheta < 0, the interpolation will take the long way around the sphere.
	// To fix this, one quat must be negated.
	if( cosTheta < 0 ) {
		z = -y;
		cosTheta = -cosTheta;
	}

	// Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero denominator
	if( cosTheta > 1 - std::numeric_limits<typename T::type_t>::epsilon() ) return uf::vector::mix( x, z, a );
	// Essential Mathematics, page 467
	typename T::type_t angle = acos(cosTheta);
/*
	return uf::vector::divide( 
		uf::vector::add(
			uf::vector::multiply(
				x,
				sin((1 - a) * angle)
			),
			uf::vector::multiply(
				z,
				sin(a * angle)
			)
		), 
		sin(angle)
	);
*/
	return (x * sin((1 - a) * angle) + z * sin(a * angle)) / sin(angle);
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
/*
	pod::Matrix4f result;

	auto qxx = q.x * q.x;
	auto qyy = q.y * q.y;
	auto qzz = q.z * q.z;
	auto qxz = q.x * q.z;
	auto qxy = q.x * q.y;
	auto qyz = q.y * q.z;
	auto qwx = q.w * q.x;
	auto qwy = q.w * q.y;
	auto qwz = q.w * q.z;

	result[0*4+0] = 1 - 2 * (qyy +  qzz);
	result[0*4+1] = 2 * (qxy + qwz);
	result[0*4+2] = 2 * (qxz - qwy);

	result[1*4+0] = 2 * (qxy - qwz);
	result[1*4+1] = 1 - 2 * (qxx +  qzz);
	result[1*4+2] = 2 * (qyz + qwx);

	result[2*4+0] = 2 * (qxz + qwy);
	result[2*4+1] = 2 * (qyz - qwx);
	result[2*4+2] = 1 - 2 * (qxx +  qyy);

	return result;
*/
	T normal = uf::quaternion::normalize( q );
	normal.w *= -1;

	typename T::type_t x = normal.x;
	typename T::type_t y = normal.y;
	typename T::type_t z = normal.z;
	typename T::type_t w = normal.w;

	typename T::type_t x2 = x * x;
	typename T::type_t y2 = y * y;
	typename T::type_t z2 = z * z;
//	typename T::type_t w2 = w * w;

	pod::Matrix4t<typename T::type_t> matrix;
	typename T::type_t mat[] = {
		1 - 2 * y2 - 2 * z2, 		2 * x * y - 2 * w * z, 		2 * x * z + 2 * w * y, 	0,
		2 * x * y + 2 * w * z, 		1 - 2 * x2 - 2 * z2, 		2 * y * z - 2 * w * x, 	0,
		2 * x * z - 2 * w * y, 		2 * y * z + 2 * w * x, 		1 - 2 * x2 - 2 * y2, 	0,
		0, 							0, 							0, 						1
	};
	for ( std::size_t i = 0; i < 16; ++i )
		matrix[i] = mat[i];
	return matrix;
}
template<typename T> pod::Quaternion<T> uf::quaternion::axisAngle( const pod::Vector3t<T>& axis, T angle ) { 
	pod::Quaternion<T> q;

	T sinAngle = sin( angle * 0.5 );
	T cosAngle = cos( angle * 0.5 );
	q.x = axis.x * sinAngle;
	q.y = axis.y * sinAngle;
	q.z = axis.z * sinAngle;
	q.w = cosAngle;
	uf::quaternion::normalize(q);
	return q;
}
template<typename T> pod::Quaternion<T> uf::quaternion::unitVectors( const pod::Vector3t<T>& u, const pod::Vector3t<T>& v ) {
	T dot = uf::vector::dot(u, v);
	if ( dot + 1.0 < 0.00001 ) return uf::quaternion::axisAngle( uf::vector::normalize(u), (T) 3.1415926 );
	T mag = sqrt( 2.0 + 2.0 * dot );
	pod::Vector3t<T> w = uf::vector::multiply(uf::vector::cross(u, v), (1.0 / mag));
	return {
		.x = w.x,
		.y = w.y,
		.z = w.z,
		.w = mag * 0.5
	};
}
template<typename T> pod::Quaternion<T> uf::quaternion::lookAt( const pod::Vector3t<T>& at, const pod::Vector3t<T>& _up ) { 
/*
	pod::Vector3 forward = uf::vector::normalize(destination - source);
	return uf::quaternion::unitVectors({0,0,1}, forward);
*/	
/*
	pod::Vector3t<T> forward = uf::vector::normalize(uf::vector::subtract( destination, source ));
	T dot = uf::vector::dot( {0, 0, -1}, forward );
	T eps = 0.000001f;
	if ( fabs(dot + 1) < eps ) return uf::quaternion::axisAngle( {0, 1, 0}, 3.1415926f );
	if ( fabs(dot - 1) < eps ) return uf::quaternion::identity<T>();
	T angle = acos(dot);
	pod::Vector3t<T> axis = uf::vector::normalize(uf::vector::cross( {0, 0, 1}, forward ));
	return uf::quaternion::axisAngle(axis, angle);
*/
	pod::Vector3t<T> up = _up;
	pod::Vector3t<T> forward = uf::vector::normalize( at ) ;
	uf::vector::orthonormalize( up, forward );
	pod::Vector3t<T> right = uf::vector::cross( up, forward );
	pod::Quaternion<T> q;
	q.w = sqrtf(1.0f + right.x + up.y + forward.z) * 0.5f;
	float w4_recip = 1.0f / (4.0f * q.w);
	q.x = (forward.y - up.z) * w4_recip;
	q.y = (right.z - forward.x) * w4_recip;
	q.z = (up.x - right.y) * w4_recip;
	return uf::quaternion::inverse( uf::quaternion::normalize( q ) );
//	return q;
}

template<typename T> T& uf::quaternion::conjugate( T& q ) {
	return q = {
		.x = -q.x,	
		.y = -q.y,	
		.z = -q.z,	
		.w =  q.w	
	};
}
template<typename T> T uf::quaternion::conjugate( const T& q ) {
	return {
		.x = -q.x,	
		.y = -q.y,	
		.z = -q.z,	
		.w =  q.w	
	};
}
template<typename T> T& uf::quaternion::inverse( T& q ) {
	return q = uf::quaternion::conjugate( (const T&) q ) / uf::quaternion::dot( q, q );
}
template<typename T> T uf::quaternion::inverse( const T& q ) {
	return uf::quaternion::conjugate( q ) / uf::quaternion::dot( q, q );
}
template<typename T> pod::Quaternion<T> uf::quaternion::fromMatrix( const pod::Matrix4t<T>& m ) {
	pod::Quaternion<T> q;

	q.w = sqrt(fmax(0, 1 + m[(4*0)+0] + m[(4*1)+1] + m[(4*2)+2])) / 2;
	q.x = sqrt(fmax(0, 1 + m[(4*0)+0] - m[(4*1)+1] - m[(4*2)+2])) / 2;
	q.y = sqrt(fmax(0, 1 - m[(4*0)+0] + m[(4*1)+1] - m[(4*2)+2])) / 2;
	q.z = sqrt(fmax(0, 1 - m[(4*0)+0] - m[(4*1)+1] + m[(4*2)+2])) / 2;

	q.x = copysign(q.x, m[(4*1)+2] - m[(4*2)+1]);
	q.y = copysign(q.y, m[(4*2)+0] - m[(4*0)+2]);
	q.z = copysign(q.z, m[(4*0)+1] - m[(4*1)+0]);
	return q;
}