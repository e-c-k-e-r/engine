template<typename T>
constexpr bool simd_able_v = std::is_same_v<T, float> || std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>;

template<size_t N, typename F>
constexpr void for_each_index(F&& f) {
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		(f(std::integral_constant<std::size_t, I>{}), ...);
	}(std::make_index_sequence<N>{});
}

#if UF_USE_SIMD
	#include "simd.h"
#endif

#define FOR_EACH( N, F ) for_each_index<N>([&](auto i) F )
#define FOR_EACH_( N, F ) for ( auto i = 0; i < N; ++i ) F;

template<typename T, typename Op>
T elementwise( const T& left, const T& right, Op&& op ) {
	T res;
	FOR_EACH(T::size, {
		res[i] = op(left[i], right[i]);
	});
	return res;
}

template<typename T>
pod::Vector1t<T> uf::vector::create( T x ) {
	return pod::Vector1t<T>{ x };
}
template<typename T>
pod::Vector2t<T> uf::vector::create( T x, T y ) {
	return pod::Vector2t<T>{ x, y };
}
template<typename T>
pod::Vector3t<T> uf::vector::create( T x, T y, T z ) {
	return pod::Vector3t<T>{ x, y, z };
}
template<typename T>
pod::Vector4t<T> uf::vector::create( T x, T y, T z, T w ) {
	return pod::Vector4t<T>{ x, y, z, w };
}
template<typename T, size_t N>
pod::Vector<T, N> uf::vector::copy( const pod::Vector<T, N>& v ) {
	return v;
}
template<typename T, size_t N>
pod::Vector<T, N> uf::vector::copy( const T* p ) {
	return *((pod::Vector<T, N>*) p);
}
template<typename T, size_t N, typename U>
pod::Vector<T, N> uf::vector::cast( const U& from ) {
	pod::Vector<T, N> to = {};
	#pragma unroll // GCC unroll N
	for ( auto i = 0; i < N && i < U::size; ++i )
		to[i] = from[i];
	return to;
}
template<typename T, size_t N>
pod::Vector<T, N> uf::vector::fill( const T v ) {
	pod::Vector<T, N> res;
	FOR_EACH(T::size, {
		res[i] = v;
	});
	return res;
}
template<typename T>
bool uf::vector::equals( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::all( uf::simd::equals( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] == right[i]) ) result = false;
	});
	return result;
}
template<typename T>
bool uf::vector::notEquals( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::any( uf::simd::notEquals( left, right ) );
	}
#endif
	bool result = false;
	FOR_EACH(T::size, {
		if ( left[i] != right[i] ) result = true;
	});
	return result;
}
template<typename T>
bool uf::vector::less( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::all( uf::simd::less( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] < right[i]) ) result = false;
	});
	return result;
}
template<typename T>
bool uf::vector::lessEquals( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::all( uf::simd::lessEquals( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] <= right[i]) ) result = false;
	});
	return result;
}
template<typename T>
bool uf::vector::greater( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::all( uf::simd::greater( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] > right[i]) ) result = false;
	});
	return result;
}
template<typename T>
bool uf::vector::greaterEquals( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::all( uf::simd::greaterEquals( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] >= right[i]) ) result = false;
	});
	return result;
}
//
template<typename T>
bool uf::vector::equals( const T& left, const typename T::type_t right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> && T::size == 4 ) {
		return uf::simd::all( uf::simd::equals( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] == right) ) result = false;
	});
	return result;
}
template<typename T>
bool uf::vector::notEquals( const T& left, const typename T::type_t right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> && T::size == 4 ) {
		return uf::simd::any( uf::simd::notEquals( left, right ) );
	}
#endif
	bool result = false;
	FOR_EACH(T::size, {
		if ( left[i] != right ) result = true;
	});
	return result;
}
template<typename T>
bool uf::vector::less( const T& left, const typename T::type_t right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> && T::size == 4 ) {
		return uf::simd::all( uf::simd::less( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] < right) ) result = false;
	});
	return result;
}
template<typename T>
bool uf::vector::lessEquals( const T& left, const typename T::type_t right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> && T::size == 4 ) {
		return uf::simd::all( uf::simd::lessEquals( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] <= right) ) result = false;
	});
	return result;
}
template<typename T>
bool uf::vector::greater( const T& left, const typename T::type_t right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> && T::size == 4 ) {
		return uf::simd::all( uf::simd::greater( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] > right) ) result = false;
	});
	return result;
}
template<typename T>
bool uf::vector::greaterEquals( const T& left, const typename T::type_t right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> && T::size == 4 ) {
		return uf::simd::all( uf::simd::greaterEquals( left, right ) );
	}
#endif
	bool result = true;
	FOR_EACH(T::size, {
		if ( !(left[i] >= right) ) result = false;
	});
	return result;
}
//
template<typename T>
bool uf::vector::isValid( const T& v ) {
	return uf::vector::equals( v, v );
}
template<typename T>
T uf::vector::add( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::add( left, right );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = left[i] + right[i];
	});
	return res;
}
template<typename T>
T uf::vector::add( const T& vector, typename T::type_t scalar ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::add( vector, scalar );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = vector[i] + scalar;
	});
	return res;
}
template<typename T>
T uf::vector::add( typename T::type_t scalar, const T& vector ) {
	return uf::vector::add( vector, scalar );
}
template<typename T>
T uf::vector::subtract( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::sub( left, right );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = left[i] - right[i];
	});
	return res;
}
template<typename T>
T uf::vector::subtract( const T& vector, typename T::type_t scalar ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::sub( vector, scalar );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = vector[i] - scalar;
	});
	return res;
}
template<typename T>
T uf::vector::subtract( typename T::type_t scalar, const T& vector ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::sub( scalar, vector );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = scalar - vector[i];
	});
	return res;
}
template<typename T>
T uf::vector::multiply( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::mul( left, right );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = left[i] * right[i];
	});
	return res;
}
template<typename T>
T uf::vector::multiply( const T& vector, typename T::type_t scalar ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::mul( vector, scalar );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = vector[i] * scalar;
	});
	return res;
}
template<typename T>
T uf::vector::multiply( typename T::type_t scalar, const T& vector ) {
	return uf::vector::multiply( vector, scalar );
}
template<typename T>
T uf::vector::divide( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::div( left, right );
	}
#elif UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		T res;
		FOR_EACH(T::size, {
			res[i] = MATH_Fast_Divide( left[i], right[i] );
		});
		return res;
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = left[i] / right[i];
	});
	return res;
}
template<typename T>
T uf::vector::divide( const T& vector, typename T::type_t scalar ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::div( vector, scalar );
	}
#elif UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		T res;
		FOR_EACH(T::size, {
			res[i] = MATH_Fast_Divide( vector[i], scalar );
		});
		return res;
	}
#endif
	T res;
	float recip = 1.0f / static_cast<float>(scalar);
	FOR_EACH(T::size, {
		res[i] = vector[i] * recip;
	});
	return res;
}
template<typename T>
T uf::vector::divide( typename T::type_t scalar, const T& vector ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::div( scalar, vector );
	}
#elif UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		T res;
		FOR_EACH(T::size, {
			res[i] = MATH_Fast_Divide( scalar, vector[i] );
		});
		return res;
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = scalar / vector[i];
	});
	return res;
}
template<typename T>
typename T::type_t uf::vector::sum( const T& vector ) {
	typename T::type_t res = 0;
	FOR_EACH(T::size, {
		res += vector[i];
	});
	return res;
}
template<typename T>
typename T::type_t uf::vector::product( const T& vector ) {
	typename T::type_t res = 1;
	FOR_EACH(T::size, {
		res *= vector[i];
	});
	return res;
}
template<typename T>
T uf::vector::negate( const T& vector ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::mul( vector, -1.f );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = -vector[i];
	});
	return res;
}
template<typename T>
T uf::vector::abs( const T& vector ) {
	T res;
	FOR_EACH(T::size, {
		res[i] = std::abs( vector[i] );
	});
	return res;
}
template<typename T>
T& uf::vector::add_( T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return left = uf::vector::add( (const T&) left, right );
	}
#endif
	FOR_EACH(T::size, {
		left[i] += right[i];
	});
	return left;
}
template<typename T>
T& uf::vector::add_( T& vector, typename T::type_t scalar ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return vector = uf::vector::add( (const T&) vector, scalar );
	}
#endif
	FOR_EACH(T::size, {
		vector[i] += scalar;
	});
	return vector;
}
template<typename T>
T& uf::vector::add_( typename T::type_t scalar, T& vector ) {
	return uf::vector::add_( vector, scalar );
}
template<typename T>
T& uf::vector::subtract_( T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return left = uf::vector::subtract( (const T&) left, right );
	}
#endif
	FOR_EACH(T::size, {
		left[i] -= right[i];
	});
	return left;
}
template<typename T>
T& uf::vector::subtract_( T& vector, typename T::type_t scalar ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return vector = uf::vector::subtract( (const T&) vector, scalar );
	}
#endif
	FOR_EACH(T::size, {
		vector[i] -= scalar;
	});
	return vector;
}
template<typename T>
T& uf::vector::subtract_( typename T::type_t scalar, T& vector ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return vector = uf::vector::subtract( scalar, (const T&) vector );
	}
#endif
	FOR_EACH(T::size, {
		vector[i] = scalar - vector[i];
	});
	return vector;
}
template<typename T>
T& uf::vector::multiply_( T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return left = uf::vector::multiply( (const T&) left, right );
	}
#endif
	FOR_EACH(T::size, {
		left[i] *= right[i];
	});
	return left;
}
template<typename T>
T& uf::vector::multiply_( T& vector, typename T::type_t scalar ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return vector = uf::vector::multiply( (const T&) vector, scalar );
	}
#endif
	FOR_EACH(T::size, {
		vector[i] *= scalar;
	});
	return vector;
}
template<typename T>
T& uf::vector::multiply_( typename T::type_t scalar, T& vector ) {
	return uf::vector::multiply_( scalar, vector );
}
template<typename T>
T& uf::vector::divide_( T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return left = uf::vector::divide( (const T&) left, right );
	}
#endif
	FOR_EACH(T::size, {
		left[i] /= right[i];
	});
	return left;
}
template<typename T>
T& uf::vector::divide_( T& vector, typename T::type_t scalar ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return vector = uf::vector::divide( (const T&) vector, scalar );
	}
#endif
	FOR_EACH(T::size, {
		vector[i] /= scalar;
	});
	return vector;
}
template<typename T>
T& uf::vector::divide_( typename T::type_t scalar, T& vector ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return vector = uf::vector::divide( scalar, (const T&) vector );
	}
#endif
	FOR_EACH(T::size, {
		vector[i] = scalar / vector[i];
	});
	return vector;
}
template<typename T>
T& uf::vector::negate_( T& vector ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return vector = uf::vector::negate( (const T&) vector );
	}
#endif
	FOR_EACH(T::size, {
		vector[i] = -vector[i];
	});
	return vector;
}
template<typename T>
T& uf::vector::normalize_( T& vector ) {
	typename T::type_t norm = uf::vector::norm(vector);
	return ( norm < 1.0e-6 ) ? T{} : ( vector = uf::vector::divide((const T&) vector, norm) );
}
template<typename T>
typename T::type_t uf::vector::min( const T& v ) {
	typename T::type_t res = v[0];
	FOR_EACH(T::size - 1, {
		res = std::min( res, v[i + 1] );
	});
	return res;
}
template<typename T>
T uf::vector::min( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::min( left, right );
	}
#endif
	T res = left;
	FOR_EACH(T::size, {
		res[i] = std::min( left[i], right[i] );
	});
	return res;
}
template<typename T>
T uf::vector::min( const T& vector, typename T::type_t right ) {
	T res = vector;
	FOR_EACH(T::size, {
		res[i] = std::min( res[i], right );
	});
	return res;
}
template<typename T>
typename T::type_t uf::vector::max( const T& v ) {
	typename T::type_t res = v[0];
	FOR_EACH(T::size - 1, {
		res = std::max( res, v[i+1] );
	});
	return res;
}
template<typename T>
T uf::vector::max( const T& left, const T& right ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::max( left, right );
	}
#endif
	T res;
	FOR_EACH(T::size, {
		res[i] = std::max( left[i], right[i] );
	});
	return res;
}
template<typename T>
T uf::vector::max( const T& vector, typename T::type_t right ) {
	T res = vector;
	FOR_EACH(T::size, {
		res[i] = std::max( res[i], right );
	});
	return res;
}

template<typename T>
T uf::vector::clamp( const T& vector, const T& min, const T& max ) {
	return uf::vector::max( min, uf::vector::min( vector, max ) );
}
template<typename T>
T uf::vector::clamp( const T& vector, typename T::type_t min, typename T::type_t max ) {
	T res = vector;
	FOR_EACH(T::size, {
		res[i] = std::clamp( res[i], min, max );
	});
	return res;
}
template<typename T>
T uf::vector::pow( const T& vector, const T& v ) {
	T res = vector;
	FOR_EACH(T::size, {
		res[i] = std::pow( res[i], v[i] );
	});
	return res;
}
template<typename T>
T uf::vector::pow( const T& vector, typename T::type_t v ) {
	T res = vector;
	FOR_EACH(T::size, {
		res[i] = std::pow( res[i], v );
	});
	return res;
}
template<typename T>
T uf::vector::ceil( const T& vector ) {
	T res;
	FOR_EACH(T::size, {
		res[i] = std::ceil( vector[i] );
	});
	return res;
}
template<typename T>
T uf::vector::floor( const T& vector ) {
	T res;
	FOR_EACH(T::size, {
		res[i] = std::floor( vector[i] );
	});
	return res;
}
template<typename T>
T uf::vector::round( const T& vector ) {
	T res;
	FOR_EACH(T::size, {
		res[i] = std::round( vector[i] );
	});
	return res;
}
template<typename T>
typename T::type_t uf::vector::dot( const T& left, const T& right ) {
#if UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return MATH_fipr( UF_EZ_VEC4(left, T::size), UF_EZ_VEC4(right, T::size) );
	}
#elif UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::dot( left, right );
	}
#endif
	return uf::vector::sum(uf::vector::multiply(left, right));
}
template<typename T>
float uf::vector::angle( const T& a, const T& b ) {
	auto dot = uf::vector::dot(a, b);
	if ( dot < -1.0f ) dot = -1.0f;
	if ( dot > 1.0f ) dot = 1.0f;
	return acos(dot);
}
template<typename T>
float uf::vector::signedAngle( const T& a, const T& b, const T& axis ) {
	auto unsignedAngle = uf::vector::angle(a, b);
	float cross_x = a.y * b.z - a.z * b.y;
	float cross_y = a.z * b.x - a.x * b.z;
	float cross_z = a.x * b.y - a.y * b.x;
	float sign = (axis.x * cross_x + axis.y * cross_y + axis.z * cross_z) >= 0.0f ? 1.0f : -1.0f;
	return unsignedAngle * sign;
}
template<typename T>
T uf::vector::lerp( const T& from, const T& to, double delta, bool clamp ) {
	delta = fmax( 0, fmin(1,delta) );
	// from + ( ( to - from ) * delta )
#if UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		T res;
		FOR_EACH(T::size, {
			res[i] = MATH_Lerp( from[i], to[i], delta );
		});
		return res;
	}
#elif UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::add(from, uf::simd::mul( uf::simd::sub(to, from), (float) delta) );
	}
#endif
	return uf::vector::add(from, uf::vector::multiply( uf::vector::subtract(to, from), delta ) );
}
template<typename T>
T uf::vector::lerp( const T& from, const T& to, const T& delta, bool clamp ) {
	//delta = fmax( 0, fmin(1,delta) );
	// from + ( ( to - from ) * delta )
#if UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		T res;
		FOR_EACH(T::size, {
			res[i] = MATH_Lerp( from[i], to[i], delta[i] );
		});
		return res;
	}
#elif UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::add(from, uf::simd::mul( uf::simd::sub(to, from), delta) );
	}
#endif
	return uf::vector::add(from, uf::vector::multiply( uf::vector::subtract(to, from), delta ) );
}
template<typename T>
T uf::vector::slerp( const T& from, const T& to, double delta, bool clamp ) {
	if ( clamp ) delta = fmax( 0, fmin(1,delta) );
	typename T::type_t dot = uf::vector::dot(from, to);
	if ( dot > 0.9995f ) return uf::vector::lerp(from, to, delta, clamp);
	typename T::type_t theta = acos(dot);
	typename T::type_t sTheta = sin(theta);

	typename T::type_t w1 = sin((1.0f - delta) * theta) / sTheta;
	typename T::type_t w2 = sin( delta * theta ) / sTheta;
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::add( uf::simd::mul( from, w1 ), uf::simd::mul( to, w2 ) );
	}
#endif
	return uf::vector::add(uf::vector::multiply(from, w1), uf::vector::multiply(to, w2));
}
template<typename T>
T uf::vector::mix( const T& x, const T& y, double a, bool clamp ) {
	if ( clamp ) a = fmax( 0, fmin(1,a) );
	// x * (1.0 - a) + y * a
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::add( uf::simd::mul( x, 1.0f - (float) a ), uf::simd::mul( y, (float) a ) );
	}
#endif
	return uf::vector::add( uf::vector::multiply( x, 1 - a ), uf::vector::multiply( y, a ) );
}
template<typename T>
typename T::type_t uf::vector::distanceSquared( const T& a, const T& b ) {
#if UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		T delta = uf::vector::subtract(b, a);
		return MATH_Sum_of_Squares( UF_EZ_VEC4( delta, T::size ) );
	}
#elif UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		uf::simd::vector<typename T::type_t> delta = uf::simd::sub( b, a );
		return uf::simd::dot( delta, delta );
	}
#endif
	T delta = uf::vector::subtract( b, a );
	return uf::vector::dot( delta, delta );
}
template<typename T>
typename T::type_t uf::vector::distance( const T& a, const T& b ) {
#if UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return MATH_Fast_Sqrt(uf::vector::distanceSquared(a,b));
	}
#endif
	return sqrt( uf::vector::distanceSquared( a, b ) );
}
template<typename T>
typename T::type_t uf::vector::magnitude( const T& vector ) {
	return uf::vector::dot(vector, vector);
}
template<typename T>
typename T::type_t uf::vector::norm( const T& vector ) {
#if UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return MATH_Fast_Sqrt( uf::vector::magnitude(vector) );
	}
#endif
	return sqrt( uf::vector::magnitude(vector) );
}
template<typename T>
T uf::vector::normalize( const T& vector ) {
#if UF_USE_SIMD // causes massive frame drops
	if constexpr ( std::is_same_v<typename T::type_t, float> ) {
		return uf::simd::normalize( vector );
	}
#endif
	typename T::type_t norm = uf::vector::norm(vector);
	if ( norm < 1.0e-6 ) return vector;	
#if UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( std::is_same_v<typename T::type_t, float> ) {
		return uf::vector::multiply(vector, MATH_fsrra(norm));
	}
#endif
	return uf::vector::divide(vector, norm);
}

template<typename T>
T uf::vector::clampMagnitude( const T& v, float maxMag ) {
	T res = v;
	float magSq = uf::vector::magnitude( res );
	if ( magSq > (maxMag * maxMag) ) {
		res *= (maxMag / sqrt(magSq));
	}
	return res;
}

template<typename T>
void uf::vector::orthonormalize( T& normal, T& tangent ) {
	normal = uf::vector::normalize( normal );
	T norm = normal;
	T tan = uf::vector::normalize( tangent );
	tangent = uf::vector::subtract( tan, uf::vector::multiply( norm, uf::vector::dot( norm, tan ) ) );
	tangent = uf::vector::normalize( tangent );
}
template<typename T>
T uf::vector::orthonormalize( const T& x, const T& y ) {
	return uf::vector::normalize( uf::vector::subtract( x, uf::vector::multiply( y, uf::vector::dot( y, x ) ) ) );
}
template<typename T>
T uf::vector::cross( const T& a, const T& b ) {
#if UF_USE_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		return uf::simd::cross( a, b );
	}
#elif UF_ENV_DREAMCAST && UF_ENV_DREAMCAST_SIMD
	if constexpr ( simd_able_v<typename T::type_t> ) {
		union {
			RETURN_VECTOR_STRUCT i;
			T o;
		} u;
		u.i = MATH_Cross_Product( a.x, a.y, a.z, b.x, b.y, b.z );
		return u.o;
	}
#endif
	T res{
		a.y * b.z - b.y * a.z,
		a.z * b.x - b.z * a.x,
		a.x * b.y - b.x * a.y
	};
	return res;
}
template<typename T>
typename T::type_t uf::vector::mips( const T& size ) {
	uint32_t max = 0;
	FOR_EACH(T::size, {
		max = std::max( max, size[i] );
	});
	return static_cast<uint32_t>(std::floor(std::log2(max))) + 1;
}

template<typename T>
size_t uf::vector::hash( const T& v ) {
	size_t hash = 0;
	FOR_EACH(T::size, {
		uf::hash( hash, v[i] );
	});
	return hash;
}

template<typename T>
uf::stl::string uf::vector::toString( const T& v, const uf::stl::string& s ) {
	uf::stl::stringstream ss;
	FOR_EACH( T::size, {
		ss << v[i];
		if ( i + 1 < T::size ) ss << ", ";
	});
	return FMT_FORMAT( fmt::runtime(s), ss.str() );
}

template<typename T, size_t N>
ext::json::Value uf::vector::encode( const pod::Vector<T,N>& v, const ext::json::EncodingSettings& settings ) {
	ext::json::Value json;
	if ( settings.quantize )
		#pragma unroll // GCC unroll T::size
		for ( auto i = 0; i < N; ++i )
			json[i] = uf::math::quantizeShort( v[i] );
	else
		#pragma unroll // GCC unroll T::size
		for ( auto i = 0; i < N; ++i )
			json[i] = v[i];
	return json;
}
template<typename T, size_t N>
pod::Vector<T,N>& uf::vector::decode( const ext::json::Value& json, pod::Vector<T,N>& v ) {
	if ( ext::json::isArray(json) )
		#pragma unroll // GCC unroll T::size
		for ( auto i = 0; i < N && i < json.size(); ++i )
			v[i] = json[i].as<T>(v[i]);
	else if ( ext::json::isObject(json) ) {
		auto i = 0;
		ext::json::forEach(json, [&](const ext::json::Value& c){
			if ( i >= N ) return;
			v[i] = uf::math::unquantize( c.as<T>(v[i]) );
			++i;
		});
	}
	return v;
}
template<typename T, size_t N>
pod::Vector<T,N> uf::vector::decode( const ext::json::Value& json, const pod::Vector<T,N>& _v ) {
	pod::Vector<T,N> v = _v;
	if ( ext::json::isArray(json) )
		#pragma unroll // GCC unroll T::size
		for ( auto i = 0; i < N && i < json.size(); ++i )
			v[i] = json[i].as<T>(_v[i]);
	else if ( ext::json::isObject(json) ) {
		auto i = 0;
		ext::json::forEach(json, [&](const ext::json::Value& c){
			if ( i >= N ) return;
			v[i] = c.as<T>(_v[i]);
			++i;
		});
	}
	return v;
}