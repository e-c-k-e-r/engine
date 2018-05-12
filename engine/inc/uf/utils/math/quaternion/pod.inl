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
template<typename T> T uf::quaternion::multiply( const T& quaternion, const typename T::type_t& scalar ) {
	return uf::vector::multiply( quaternion, scalar );
}
// 	Flip sign of all components
template<typename T> T uf::quaternion::negate( const T& quaternion ) {
	return uf::quaternion::inverse(quaternion);
}
template<typename T = pod::Math::num_t> pod::Quaternion<T> uf::quaternion::identity() {
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

// 	Linearly interpolate between two quaternions
template<typename T> T uf::quaternion::lerp( const T& from, const T& to, double delta ) {
	return uf::vector::lerp( from, to, delta );
}
// 	Spherically interpolate between two quaternions
template<typename T> T uf::quaternion::slerp( const T& from, const T& to, double delta ) {
	return uf::vector::slerp( from, to, delta );
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
template<typename T> pod::Matrix4t<typename T::type_t> uf::quaternion::matrix( const T& quaternion ) {
	T normal = uf::quaternion::normalize(quaternion);
#ifdef UF_USE_GLM
	{
		glm::quat glmq;
		glmq.w = normal.w; glmq.x = normal.x; glmq.y = normal.y; glmq.z = normal.z;
		glm::mat4 mat = glm::toMat4(glmq);

		pod::Matrix4t<typename T::type_t> matrix;
		for ( int j = 0; j < 4; ++j ) for ( int i = 0; i < 4; ++i ) matrix[(j*4)+i] = mat[j][i];
		return matrix;
	}
#endif
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
#ifdef UF_USE_GLM
	{
		glm::quat glmq = glm::angleAxis(glm::radians(angle), glm::vec3(axis.x, axis.y, axis.z));
		q.w = glmq.w; q.x = glmq.x; q.y = glmq.y; q.z = glmq.z;
		return q;
	}
#endif
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
	if ( dot + 1.0 < 0.00001 ) return uf::quaternion::axisAngle( uf::vector::normalize(u), 3.1415926 );
	T mag = sqrt( 2.0 + 2.0 * dot );
	pod::Vector3t<T> w = uf::vector::multiply(uf::vector::cross(u, v), (1.0 / mag));
	return {
		.x = w.x,
		.y = w.y,
		.z = w.z,
		.w = mag * 0.5
	};
}
template<typename T> pod::Quaternion<T> uf::quaternion::lookAt( const pod::Vector3t<T>& source, const pod::Vector3t<T>& destination ) { 
	pod::Vector3 forward = uf::vector::normalize(destination - source);
	return uf::quaternion::unitVectors({0,0,1}, forward);
/*	
	pod::Vector3t<T> forward = uf::vector::normalize(uf::vector::subtract( destination, source ));
	T dot = uf::vector::dot( {0, 0, -1}, forward );
	T eps = 0.000001f;
	if ( fabs(dot + 1) < eps ) return uf::quaternion::axisAngle( {0, 1, 0}, 3.1415926 );
	if ( fabs(dot - 1) < eps ) return uf::quaternion::identity<T>();
	T angle = acos(dot);
	pod::Vector3t<T> axis = uf::vector::normalize(uf::vector::cross( {0, 0, 1}, forward ));
	return uf::quaternion::axisAngle(axis, angle);
*/
}

template<typename T> T uf::quaternion::conjugate( const T& quaternion ) {
	return {
		.x = -quaternion.x,	
		.y = -quaternion.y,	
		.z = -quaternion.z,	
		.w =  quaternion.w	
	};
}
template<typename T> T uf::quaternion::inverse( const T& quaternion ) {
	return {
		.x = -quaternion.x,	
		.y = -quaternion.y,	
		.z = -quaternion.z,	
		.w =  quaternion.w	
	};
}
template<typename T> T& uf::quaternion::conjugate( T& quaternion ) {
	return quaternion = uf::quaternion::conjugate(quaternion);
}
template<typename T> T& uf::quaternion::inverse( T& quaternion ) {
	return quaternion = uf::quaternion::inverse(quaternion);
}