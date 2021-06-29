#if !__clang__ && __GNUC__
	#pragma GCC push_options
	#pragma GCC optimize ("unroll-loops")
#endif

#if UF_USE_SIMD
	#include "simd.h"
#endif
#include "redundancy.inl"
// 	Equality checking
template<typename T>
pod::Vector1t<T> /*UF_API*/ uf::vector::create( T x ) { pod::Vector1t<T> vec; vec.x = x; return vec; }
template<typename T>
pod::Vector2t<T> /*UF_API*/ uf::vector::create( T x, T y ) { pod::Vector2t<T> vec; vec.x = x, vec.y = y; return vec; }
template<typename T>
pod::Vector3t<T> /*UF_API*/ uf::vector::create( T x, T y, T z ) { pod::Vector3t<T> vec; vec.x = x, vec.y = y, vec.z = z; return vec; }
template<typename T>
pod::Vector4t<T> /*UF_API*/ uf::vector::create( T x, T y, T z, T w ) { pod::Vector4t<T> vec; vec.x = x, vec.y = y, vec.z = z, vec.w = w; return vec; }
template<typename T, size_t N>
pod::Vector<T, N> /*UF_API*/ uf::vector::copy( const pod::Vector<T, N>& v ) { return v; }
template<typename T, size_t N, typename U>
pod::Vector<T, N> /*UF_API*/ uf::vector::cast( const U& from ) {
	alignas(16) pod::Vector<T, N> to;
	#pragma unroll // GCC unroll N
	for ( uint_fast8_t i = 0; i < N && i < U::size; ++i )
		to[i] = from[i];
	return to;
}
// 	Equality checking
template<typename T> 														// 	Equality check between two vectors (less than)
int /*UF_API*/ uf::vector::compareTo( const T& left, const T& right ) {
	return memcmp( &left, &right, T::size );
}
template<typename T> 														// 	Equality check between two vectors (equals)
bool /*UF_API*/ uf::vector::equals( const T& left, const T& right ) {
//	return uf::vector::compareTo(left, right) == 0;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		if ( left[i] != right[i] ) return false;
	return true;
}
// Basic arithmetic
template<typename T> 														// Adds two vectors of same type and size together
T /*UF_API*/ uf::vector::add( const T& left, const T& right ) {
#if UF_USE_SIMD
	return uf::simd::add( left, right );
#endif
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = left[i] + right[i];
	return res;
}
template<typename T> 														// Multiplies this vector by a scalar
T /*UF_API*/ uf::vector::add( const T& vector, const typename T::type_t& scalar ) {
#if UF_USE_SIMD
	return uf::simd::add( vector, scalar );
#endif
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = vector[i] + scalar;
	return res;
}
template<typename T> 														// Subtracts two vectors of same type and size together
T /*UF_API*/ uf::vector::subtract( const T& left, const T& right ) {
#if UF_USE_SIMD
	return uf::simd::sub( left, right );
#endif
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = left[i] - right[i];
	return res;
}
template<typename T> 														// Multiplies this vector by a scalar
T /*UF_API*/ uf::vector::subtract( const T& vector, const typename T::type_t& scalar ) {
#if UF_USE_SIMD
	return uf::simd::sub( vector, scalar );
#endif
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = vector[i] - scalar;
	return res;
}
template<typename T> 														// Multiplies two vectors of same type and size together
T /*UF_API*/ uf::vector::multiply( const T& left, const T& right ) {
#if UF_USE_SIMD
	return uf::simd::mul( left, right );
#endif
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = left[i] * right[i];
	return res;
}
template<typename T> 														// Multiplies this vector by a scalar
T /*UF_API*/ uf::vector::multiply( const T& vector, const typename T::type_t& scalar ) {
#if UF_USE_SIMD
	return uf::simd::mul( vector, scalar );
#endif
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = vector[i] * scalar;
	return res;
}
template<typename T> 														// Divides two vectors of same type and size together
T /*UF_API*/ uf::vector::divide( const T& left, const T& right ) {
#if UF_USE_SIMD
	return uf::simd::div( left, right );
#elif UF_ENV_DREAMCAST
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = MATH_Fast_Divide(left[i], right[i]);
	return res;
#else
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = left[i] / right[i];
	return res;
#endif
}
template<typename T> 														// Divides this vector by a scalar
T /*UF_API*/ uf::vector::divide( const T& vector, const typename T::type_t& scalar ) {
#if UF_USE_SIMD
	return uf::simd::div( vector, scalar );
#elif UF_ENV_DREAMCAST
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = MATH_Fast_Divide(vector[i], scalar);
	return res;
#else
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = vector[i] / scalar;
	return res;
#endif
}
template<typename T> 														// Compute the sum of all components 
typename T::type_t /*UF_API*/ uf::vector::sum( const T& vector ) {
	typename T::type_t res = 0;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res += vector[i];
	return res;
}
template<typename T> 														// Compute the product of all components 
typename T::type_t /*UF_API*/ uf::vector::product( const T& vector ) {
	typename T::type_t res = 0;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res *= vector[i];
	return res;
}
template<typename T> 														// Flip sign of all components
T /*UF_API*/ uf::vector::negate( const T& vector ) {
#if UF_USE_SIMD
	return uf::simd::mul( vector, -1.f );
#endif
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = -vector[i];
	return res;
}
// Writes to first value
template<typename T> 														// Adds two vectors of same type and size together
T& /*UF_API*/ uf::vector::add( T& left, const T& right ) {
#if UF_USE_SIMD
	return left = uf::vector::add( (const T&) left, right );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		left[i] += right[i];
	return left;
}
template<typename T> 														// Multiplies this vector by a scalar
T& /*UF_API*/ uf::vector::add( T& vector, const typename T::type_t& scalar ) {
#if UF_USE_SIMD
	return vector = uf::vector::add( (const T&) vector, scalar );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		vector[i] += scalar;
	return vector;
}
template<typename T> 														// Subtracts two vectors of same type and size together
T& /*UF_API*/ uf::vector::subtract( T& left, const T& right ) {
#if UF_USE_SIMD
	return left = uf::vector::subtract( (const T&) left, right );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		left[i] -= right[i];
	return left;
}
template<typename T> 														// Multiplies this vector by a scalar
T& /*UF_API*/ uf::vector::subtract( T& vector, const typename T::type_t& scalar ) {
#if UF_USE_SIMD
	return vector = uf::vector::subtract( (const T&) vector, scalar );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		vector[i] -= scalar;
	return vector;
}
template<typename T> 														// Multiplies two vectors of same type and size together
T& /*UF_API*/ uf::vector::multiply( T& left, const T& right ) {
#if UF_USE_SIMD
	return left = uf::vector::multiply( (const T&) left, right );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		left[i] *= right[i];
	return left;
}
template<typename T> 														// Multiplies this vector by a scalar
T& /*UF_API*/ uf::vector::multiply( T& vector, const typename T::type_t& scalar ) {
#if UF_USE_SIMD
	return vector = uf::vector::multiply( (const T&) vector, scalar );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		vector[i] *= scalar;
	return vector;
}
template<typename T> 														// Divides two vectors of same type and size together
T& /*UF_API*/ uf::vector::divide( T& left, const T& right ) {
#if UF_USE_SIMD
	return left = uf::vector::divide( (const T&) left, right );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		left[i] /= right[i];
	return left;
}
template<typename T> 														// Divides this vector by a scalar
T& /*UF_API*/ uf::vector::divide( T& vector, const typename T::type_t& scalar ) {
#if UF_USE_SIMD
	return vector = uf::vector::divide( (const T&) vector, scalar );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		vector[i] /= scalar;
	return vector;
}
template<typename T> 														// Flip sign of all components
T& /*UF_API*/ uf::vector::negate( T& vector ) {
#if UF_USE_SIMD
	return vector = uf::vector::negate( (const T&) vector );
#endif
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		vector[i] = -vector[i];
	return vector;
}
template<typename T> 														// Normalizes a vector
T& /*UF_API*/ uf::vector::normalize( T& vector ) {
	typename T::type_t norm = uf::vector::norm(vector);
	if ( norm == 0 ) return vector;	
	return uf::vector::divide(vector, norm);
}
// Complex arithmetic
template<typename T> 														// Compute the dot product between two vectors
typename T::type_t /*UF_API*/ uf::vector::dot( const T& left, const T& right ) {
#if UF_ENV_DREAMCAST
	return MATH_fipr( UF_EZ_VEC4(left, T::size), UF_EZ_VEC4(right, T::size) );
#elif UF_USE_SIMD
	return uf::simd::dot( left, right );
#endif
	return uf::vector::sum(uf::vector::multiply(left, right));
}
template<typename T> 														// Compute the angle between two vectors
pod::Angle /*UF_API*/ uf::vector::angle( const T& a, const T& b ) {
	return acos(uf::vector::dot(a, b));
}
template<typename T> 														// Linearly interpolate between two vectors
T /*UF_API*/ uf::vector::lerp( const T& from, const T& to, double delta, bool clamp ) {
	delta = fmax( 0, fmin(1,delta) );
	// from + ( ( to - from ) * delta )
#if UF_ENV_DREAMCAST
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = MATH_Lerp( from[i], to[i], delta );
	return res;
#elif UF_USE_SIMD
	return uf::simd::add(from, uf::simd::mul( uf::simd::sub(to, from), (float) delta) );
#endif
	return uf::vector::add(from, uf::vector::multiply( uf::vector::subtract(to, from), delta ) );
}
template<typename T> 														// Linearly interpolate between two vectors
T /*UF_API*/ uf::vector::lerp( const T& from, const T& to, const T& delta, bool clamp ) {
	//delta = fmax( 0, fmin(1,delta) );
	// from + ( ( to - from ) * delta )
#if UF_ENV_DREAMCAST
	alignas(16) T res;
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < T::size; ++i )
		res[i] = MATH_Lerp( from[i], to[i], delta[i] );
	return res;
#elif UF_USE_SIMD
	return uf::simd::add(from, uf::simd::mul( uf::simd::sub(to, from), delta) );
#endif
	return uf::vector::add(from, uf::vector::multiply( uf::vector::subtract(to, from), delta ) );
}
template<typename T> 														// Spherically interpolate between two vectors
T /*UF_API*/ uf::vector::slerp( const T& from, const T& to, double delta, bool clamp ) {
	if ( clamp ) delta = fmax( 0, fmin(1,delta) );
	typename T::type_t dot = uf::vector::dot(from, to);
	typename T::type_t theta = acos(dot);
	typename T::type_t sTheta = sin(theta);

	typename T::type_t w1 = sin((1.0f - delta) * theta / sTheta);
	typename T::type_t w2 = sin( delta * theta / sTheta );
#if UF_USE_SIMD
	return uf::simd::add( uf::simd::mul( from, w1 ), uf::simd::mul( to, w2 ) );
#endif
	return uf::vector::add(uf::vector::multiply(from, w1), uf::vector::multiply(to, w2));
}
template<typename T> 														// 
T /*UF_API*/ uf::vector::mix( const T& x, const T& y, double a, bool clamp ) {
	if ( clamp ) a = fmax( 0, fmin(1,a) );
	// x * (1.0 - a) + y * a
#if UF_USE_SIMD
	return uf::simd::add( uf::simd::mul( x, 1.0f - (float) a ), uf::simd::mul( y, (float) a ) );
#endif
	return uf::vector::add( uf::vector::multiply( x, 1 - a ), uf::vector::multiply( y, a ) );
}
template<typename T> 														// Compute the distance between two vectors (doesn't sqrt)
typename T::type_t /*UF_API*/ uf::vector::distanceSquared( const T& a, const T& b ) {
#if UF_ENV_DREAMCAST
	alignas(16) T delta = uf::vector::subtract(b, a);
	return MATH_Sum_of_Squares( UF_EZ_VEC4( delta, T::size ) );
#elif UF_USE_SIMD
	uf::simd::value<typename T::type_t> delta = uf::simd::sub( b, a );
	return uf::vector::sum( uf::simd::vector( uf::simd::mul( delta, delta ) ) );
#else
	alignas(16) T delta = uf::vector::subtract(b, a);
	uf::vector::multiply( delta, delta );
	return uf::vector::sum(delta);
#endif
}
template<typename T> 														// Compute the distance between two vectors
typename T::type_t /*UF_API*/ uf::vector::distance( const T& a, const T& b ) {
#if UF_ENV_DREAMCAST
	return MATH_Fast_Sqrt(uf::vector::distanceSquared(a,b));
#endif
	return sqrt(uf::vector::distanceSquared(a,b));
}
template<typename T> 														// Gets the magnitude of the vector
typename T::type_t /*UF_API*/ uf::vector::magnitude( const T& vector ) {
	return uf::vector::dot(vector, vector);
}
template<typename T> 														// Compute the norm of the vector
typename T::type_t /*UF_API*/ uf::vector::norm( const T& vector ) {
#if UF_ENV_DREAMCAST
	return MATH_Fast_Sqrt( uf::vector::magnitude(vector) );
#endif
	return sqrt( uf::vector::magnitude(vector) );
}
template<typename T> 														// Normalizes a vector
T /*UF_API*/ uf::vector::normalize( const T& vector ) {
	typename T::type_t norm = uf::vector::norm(vector);
	if ( norm == 0 ) return vector;	
#if UF_ENV_DREAMCAST
	return uf::vector::multiply(vector, MATH_fsrra(norm));
#endif
	return uf::vector::divide(vector, norm);
}
template<typename T> 														// Normalizes a vector
void /*UF_API*/ uf::vector::orthonormalize( T& normal, T& tangent ) {
	normal = uf::vector::normalize( normal );
	alignas(16) T norm = normal;
	alignas(16) T tan = uf::vector::normalize( tangent );
	tangent = uf::vector::subtract( tan, uf::vector::multiply( norm, uf::vector::dot( norm, tan ) ) );
	tangent = uf::vector::normalize( tangent );
}
template<typename T> 														// Normalizes a vector
T /*UF_API*/ uf::vector::orthonormalize( const T& x, const T& y ) {
	return uf::vector::normalize( uf::vector::subtract( x, uf::vector::multiply( y, uf::vector::dot( y, x ) ) ) );
}
template<typename T> 														// Normalizes a vector
T /*UF_API*/ uf::vector::cross( const T& a, const T& b ) {
#if UF_USE_SIMD
	uf::simd::value<typename T::type_t> x = a;
	uf::simd::value<typename T::type_t> y = b;
	#if SSE_INSTR_SET >= 7
		uf::simd::value<typename T::type_t> tmp0 = _mm_shuffle_ps(y,y,_MM_SHUFFLE(3,0,2,1));
		uf::simd::value<typename T::type_t> tmp1 = _mm_shuffle_ps(x,x,_MM_SHUFFLE(3,0,2,1));
		tmp1 = _mm_mul_ps(tmp1,y);
		uf::simd::value<typename T::type_t> tmp2 = _mm_fmsub_ps( tmp0,x, tmp1 );
		uf::simd::value<typename T::type_t> res = _mm_shuffle_ps(tmp2,tmp2,_MM_SHUFFLE(3,0,2,1));
		return res;
	#else
		uf::simd::value<typename T::type_t> tmp0 = _mm_shuffle_ps(y,y,_MM_SHUFFLE(3,0,2,1));
		uf::simd::value<typename T::type_t> tmp1 = _mm_shuffle_ps(x,x,_MM_SHUFFLE(3,0,2,1));
		tmp0 = _mm_mul_ps(tmp0,x);
		tmp1 = _mm_mul_ps(tmp1,y);
		uf::simd::value<typename T::type_t> tmp2 = _mm_sub_ps(tmp0,tmp1);
		uf::simd::value<typename T::type_t> res = _mm_shuffle_ps(tmp2,tmp2,_MM_SHUFFLE(3,0,2,1));
		return res;
	#endif
#elif UF_ENV_DREAMCAST
	auto res = MATH_Cross_Product( a.x, a.y, a.z, b.x, b.y, b.z );
	return *((T*) &res);
#else
	alignas(16) T res{
		a.y * b.z - b.y * a.z,
		a.z * b.x - b.z * a.x,
		a.x * b.y - b.x * a.y
	}
	return res;
#endif
}
template<typename T> 														// Normalizes a vector
uf::stl::string /*UF_API*/ uf::vector::toString( const T& v ) {
	uint_fast8_t size = T::size;
	uf::stl::stringstream ss;
	ss << "Vector(";
	#pragma unroll // GCC unroll T::size
	for ( uint_fast8_t i = 0; i < size; ++i ) {
		ss << v[i];
		if ( i + 1 < size ) ss << ", ";
	}
	ss << ")";
	return ss.str();
}

template<typename T, size_t N>
ext::json::Value /*UF_API*/ uf::vector::encode( const pod::Vector<T,N>& v, const ext::json::EncodingSettings& settings ) {
	ext::json::Value json;
	if ( settings.quantize )
		#pragma unroll // GCC unroll T::size
		for ( uint_fast8_t i = 0; i < N; ++i )
			json[i] = uf::math::quantizeShort( v[i] );
	else
		#pragma unroll // GCC unroll T::size
		for ( uint_fast8_t i = 0; i < N; ++i )
			json[i] = v[i];
	return json;
}
template<typename T, size_t N>
pod::Vector<T,N>& /*UF_API*/ uf::vector::decode( const ext::json::Value& json, pod::Vector<T,N>& v ) {
	if ( ext::json::isArray(json) )
		#pragma unroll // GCC unroll T::size
		for ( uint_fast8_t i = 0; i < N; ++i )
			v[i] = json[i].as<T>(v[i]);
	else if ( ext::json::isObject(json) ) {
		uint_fast8_t i = 0;
		ext::json::forEach(json, [&](const ext::json::Value& c){
			if ( i >= N ) return;
			v[i] = uf::math::unquantize( c.as<T>(v[i]) );
			++i;
		});
	}
	return v;
}
template<typename T, size_t N>
pod::Vector<T,N> /*UF_API*/ uf::vector::decode( const ext::json::Value& json, const pod::Vector<T,N>& _v ) {
	pod::Vector<T,N> v = _v;
	if ( ext::json::isArray(json) )
		#pragma unroll // GCC unroll T::size
		for ( uint_fast8_t i = 0; i < N; ++i )
			v[i] = json[i].as<T>(_v[i]);
	else if ( ext::json::isObject(json) ) {
		uint_fast8_t i = 0;
		ext::json::forEach(json, [&](const ext::json::Value& c){
			if ( i >= N ) return;
			v[i] = c.as<T>(_v[i]);
			++i;
		});
	}
	return v;
}
#if !__clang__ && __GNUC__
	#pragma GCC pop_options
#endif