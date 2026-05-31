#pragma once

#include <uf/config.h>

#include "math.h"

#include <sstream>
#include <cmath>
#include <cstring>
#include <array>
#include <algorithm>
#include <cstddef>
#include <stdint.h>

#include <uf/utils/memory/vector.h>
#include <uf/ext/json/json.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/math/angle.h>
#include <uf/utils/math/hash.h>

#if UF_USE_BFLOAT16
	#include <stdfloat>
#endif

namespace pod {
	template<typename T, size_t N>
	struct Vector;

	template<typename T = float> using Vector1t = Vector<T,1>;
	typedef Vector1t<NUM> Vector1;
	typedef Vector1t<int16_t> Vector1s;
	typedef Vector1t<uint16_t> Vector1us;
	typedef Vector1t<int32_t> Vector1i;
	typedef Vector1t<uint32_t> Vector1ui;

	typedef Vector1t<long> Vector1l;
	typedef Vector1t<float> Vector1f;
	typedef Vector1t<double> Vector1d;

	template<typename T = float> using Vector2t = Vector<T,2>;
	typedef Vector2t<NUM> Vector2;
	typedef Vector2t<int16_t> Vector2s;
	typedef Vector2t<uint16_t> Vector2us;
	typedef Vector2t<int32_t> Vector2i;
	typedef Vector2t<uint32_t> Vector2ui;

	typedef Vector2t<long> Vector2l;
	typedef Vector2t<float> Vector2f;
	typedef Vector2t<double> Vector2d;

	template<typename T = float> using Vector3t = Vector<T,3>;
	typedef Vector3t<NUM> Vector3;
	typedef Vector3t<uint8_t> Vector3b;
	typedef Vector3t<int16_t> Vector3s;
	typedef Vector3t<uint16_t> Vector3us;
	typedef Vector3t<int32_t> Vector3i;
	typedef Vector3t<uint32_t> Vector3ui;

	typedef Vector3t<long> Vector3l;
	typedef Vector3t<float> Vector3f;
	typedef Vector3t<double> Vector3d;

	template<typename T = float> using Vector4t = Vector<T,4>;
	typedef Vector4t<NUM> Vector4;
	typedef Vector4t<uint8_t> Vector4b;
	typedef Vector4t<int16_t> Vector4s;
	typedef Vector4t<uint16_t> Vector4us;
	typedef Vector4t<int32_t> Vector4i;
	typedef Vector4t<uint32_t> Vector4ui;

	typedef Vector4t<long> Vector4l;
	typedef Vector4t<float> Vector4f;
	typedef Vector4t<double> Vector4d;

#if UF_USE_FLOAT16
	typedef Vector2t<std::float16_t> Vector2f16;
	typedef Vector3t<std::float16_t> Vector3f16;
	typedef Vector4t<std::float16_t> Vector4f16;
#endif

#if UF_USE_BFLOAT16
	typedef Vector2t<std::bfloat16_t> Vector2bf16;
	typedef Vector3t<std::bfloat16_t> Vector3bf16;
	typedef Vector4t<std::bfloat16_t> Vector4bf16;
#endif
}


// 	POD vector accessing/manipulation
namespace uf {
	namespace vector {
		template<typename T> /*FORCE_INLINE*/ pod::Vector1t<T> /*UF_API*/ create( T x ); // creates a 1D vector
		template<typename T> /*FORCE_INLINE*/ pod::Vector2t<T> /*UF_API*/ create( T x, T y ); // creates a 2D vector
		template<typename T> /*FORCE_INLINE*/ pod::Vector3t<T> /*UF_API*/ create( T x, T y, T z ); // creates a 3D vector
		template<typename T> /*FORCE_INLINE*/ pod::Vector4t<T> /*UF_API*/ create( T x, T y, T z, T w ); // creates a 4D vector
		template<typename T, size_t N> /*FORCE_INLINE*/ pod::Vector<T, N> /*UF_API*/ copy( const pod::Vector<T, N>& = {}); // creates a copy of a vector (for whatever reason)
		template<typename T, size_t N> /*FORCE_INLINE*/ pod::Vector<T, N> /*UF_API*/ copy( const T* ); // creates a copy of a vector (for whatever reason)
		template<typename T, size_t N, typename U> /*FORCE_INLINE*/ pod::Vector<T, N> /*UF_API*/ cast( const U& from ); // casts one vector of one type to another (of the same size)
	// 	Equality checking
		template<typename T> /*FORCE_INLINE*/ bool /*UF_API*/ equals( const T& left, const T& right ); // equality check between two vectors (==)
		template<typename T> /*FORCE_INLINE*/ bool /*UF_API*/ notEquals( const T& left, const T& right ); // equality check between two vectors (==)
		template<typename T> /*FORCE_INLINE*/ bool /*UF_API*/ less( const T& left, const T& right ); // equality check between two vectors (<)
		template<typename T> /*FORCE_INLINE*/ bool /*UF_API*/ lessEquals( const T& left, const T& right ); // equality check between two vectors (<=)
		template<typename T> /*FORCE_INLINE*/ bool /*UF_API*/ greater( const T& left, const T& right ); // equality check between two vectors (>)
		template<typename T> /*FORCE_INLINE*/ bool /*UF_API*/ greaterEquals( const T& left, const T& right ); // equality check between two vectors (>=)

		template<typename T> /*FORCE_INLINE*/ bool /*UF_API*/ isValid( const T& v ); // checks if all components are valid (non NaN, inf, etc.)
	// 	Basic arithmetic
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ add( const T& left, const T& right ); // adds two vectors of same type and size together
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ add( const T& left, typename T::type_t scalar ); // adds a scalar to every component of the vector
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ add( typename T::type_t scalar, const T& vector ); // adds a scalar to every component of the vector (inverted)
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ subtract( const T& left, const T& right ); // subtracts two vectors of same type and size together
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ subtract( const T& left, typename T::type_t scalar ); // subtracts a scalar to every component of the vector
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ subtract( typename T::type_t scalar, const T& vector ); // subtracts a scalar to every component of the vector (inverted)
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ multiply( const T& left, const T& right ); // multiplies two vectors of same type and size together
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ multiply( const T& vector, typename T::type_t scalar ); // multiplies a scalar to every component of the vector
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ multiply( typename T::type_t scalar, const T& vector ); // multiplies a scalar to every component of the vector (inverted)
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ divide( const T& left, const T& right ); // divides two vectors of same type and size together
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ divide( const T& left, typename T::type_t scalar ); // divides a scalar to every component of the vector
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ divide( typename T::type_t scalar, const T& vector ); // divides a scalar to every component of the vector (inverted)

		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ add_( T& left, const T& right ); // adds two vectors of same type and size together
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ add_( T& left, typename T::type_t scalar ); // adds a scalar to every component of the vector
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ add_( typename T::type_t scalar, T& vector ); // adds a scalar to every component of the vector (inverted)
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ subtract_( T& left, const T& right ); // subtracts two vectors of same type and size together
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ subtract_( T& left, typename T::type_t scalar ); // subtracts a scalar to every component of the vector
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ subtract_( typename T::type_t scalar, T& vector ); // subtracts a scalar to every component of the vector (inverted)
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ multiply_( T& left, const T& right ); // multiplies two vectors of same type and size together
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ multiply_( T& vector, typename T::type_t scalar ); // multiplies a scalar to every component of the vector
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ multiply_( typename T::type_t scalar, T& vector ); // multiplies a scalar to every component of the vector (inverted)
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ divide_( T& left, const T& right ); // divides two vectors of same type and size together
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ divide_( T& left, typename T::type_t scalar ); // divides a scalar to every component of the vector
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ divide_( typename T::type_t scalar, T& vector ); // divides a scalar to every component of the vector (inverted)
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ negate_( T& vector ); // flip sign of all components
		template<typename T> /*FORCE_INLINE*/ T& /*UF_API*/ normalize_( T& vector ); // normalizes a vector

		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ sum( const T& vector ); // compute the sum of all components 
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ product( const T& vector ); // compute the product of all components 
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ negate( const T& vector ); // flip sign of all components
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ abs( const T& vector ); // gets the absolute value of a vector
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ min( const T& left, const T& right ); // returns the minimum of each component between two vectors
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ max( const T& left, const T& right ); // returns the maximum of each component between two vectors
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ min( const T& v ); // returns the minimum component of a vector
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ max( const T& v ); // returns the maximum component of a vector
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ min( const T& vector, typename T::type_t& min ); // applies min on a vector
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ max( const T& vector, typename T::type_t& max ); // applies max on a vector
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ clamp( const T& vector, const T& min, const T& max ); // clamps a vector between two bounds
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ clamp( const T& vector, typename T::type_t& min, typename T::type_t& max ); // clamps a vector between a bound
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ ceil( const T& vector ); // rounds each component of the the vector up
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ floor( const T& vector ); // rounds each component of the vector down
		template<typename T> /*FORCE_INLINE*/ T  /*UF_API*/ round( const T& vector );  // rounds each component of the vector
	// 	Complex arithmetic
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ dot( const T& left, const T& right ); // compute the dot product between two vectors
		template<typename T> /*FORCE_INLINE*/ float /*UF_API*/ angle( const T& a, const T& b ); // compute the angle between two vectors
		template<typename T> /*FORCE_INLINE*/ float /*UF_API*/ signedAngle( const T& a, const T& b, const T& axis ); // compute the signed angle between two vectors
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ cross( const T& a, const T& b ); // compute the cross product between two vectors
		
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ lerp( const T& from, const T& to, double, bool = true ); // linearly interpolate between two vectors
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ lerp( const T& from, const T& to, const T&, bool = true ); // linearly interpolate between two vectors, component-wise

		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ slerp( const T& from, const T& to, double, bool = false ); // spherically interpolate between two vectors
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ mix( const T& from, const T& to, double, bool = false );   	
		
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ distanceSquared( const T& a, const T& b ); // compute the distance between two vectors (doesn't sqrt)
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ distance( const T& a, const T& b ); // compute the distance between two vectors
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ magnitude( const T& vector ); // gets the magnitude of the vector
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ norm( const T& vector ); // compute the norm (length) of the vector
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ normalize( const T& vector ); // normalizes a vector
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ clampMagnitude( const T& vector, float maxMag ); // clamps the magnitude of a vector
		template<typename T> /*FORCE_INLINE*/ void /*UF_API*/ orthonormalize( T& x, T& y ); // orthonormalizes a vector against another vector
		template<typename T> /*FORCE_INLINE*/ T /*UF_API*/ orthonormalize( const T& x, const T& y ); // orthonormalizes a vector against another vector
		
		template<typename T> /*FORCE_INLINE*/ size_t /*UF_API*/ hash( const T& vector ); // hashes a vector
		template<typename T> /*FORCE_INLINE*/ uf::stl::string /*UF_API*/ toString( const T& vector ); // parses a vector as a string
		template<typename T, size_t N> /*FORCE_INLINE*/ ext::json::Value encode( const pod::Vector<T,N>& v, const ext::json::EncodingSettings& = {} ); // parses a vector into a JSON value
		
		template<typename T, size_t N> /*FORCE_INLINE*/ pod::Vector<T,N>& decode( const ext::json::Value& v, pod::Vector<T,N>& ); // parses a JSON value into a vector
		template<typename T, size_t N> /*FORCE_INLINE*/ pod::Vector<T,N> decode( const ext::json::Value& v, const pod::Vector<T,N>& = {} ); // parses a JSON value into a vector
		
		template<typename T> /*FORCE_INLINE*/ typename T::type_t /*UF_API*/ mips( const T& size ); // calculate amount of mips to use given a size
	}
}

#define DEFINE_VECTOR(T, N) \
	using type_t = T;\
	using container_t = T*;\
	static constexpr size_t size = N;\
	FORCE_INLINE T& operator[](size_t i) { return components[i]; }\
	FORCE_INLINE const T& operator[](size_t i) const { return components[i]; }\
	FORCE_INLINE Vector<T,N> operator-() const { return uf::vector::negate(*this); } \
	FORCE_INLINE Vector<T,N> operator+(const Vector<T,N>& rhs) const { return uf::vector::add(*this, rhs); } \
	FORCE_INLINE Vector<T,N> operator-(const Vector<T,N>& rhs) const { return uf::vector::subtract(*this, rhs); } \
	FORCE_INLINE Vector<T,N> operator*(const Vector<T,N>& rhs) const { return uf::vector::multiply(*this, rhs); } \
	FORCE_INLINE Vector<T,N> operator/(const Vector<T,N>& rhs) const { return uf::vector::divide(*this, rhs); } \
	FORCE_INLINE Vector<T,N> operator+(type_t scalar) const { return uf::vector::add(*this, scalar); } \
	FORCE_INLINE Vector<T,N> operator-(type_t scalar) const { return uf::vector::subtract(*this, scalar); } \
	FORCE_INLINE Vector<T,N> operator*(type_t scalar) const { return uf::vector::multiply(*this, scalar); } \
	FORCE_INLINE Vector<T,N> operator/(type_t scalar) const { return uf::vector::divide(*this, scalar); } \
	FORCE_INLINE Vector<T,N>& operator+=(const Vector<T,N>& rhs) { return uf::vector::add_(*this, rhs); } \
	FORCE_INLINE Vector<T,N>& operator-=(const Vector<T,N>& rhs) { return uf::vector::subtract_(*this, rhs); } \
	FORCE_INLINE Vector<T,N>& operator*=(const Vector<T,N>& rhs) { return uf::vector::multiply_(*this, rhs); } \
	FORCE_INLINE Vector<T,N>& operator/=(const Vector<T,N>& rhs) { return uf::vector::divide_(*this, rhs); } \
	FORCE_INLINE Vector<T,N>& operator+=(type_t scalar) { return uf::vector::add_(*this, scalar); } \
	FORCE_INLINE Vector<T,N>& operator-=(type_t scalar) { return uf::vector::subtract_(*this, scalar); } \
	FORCE_INLINE Vector<T,N>& operator*=(type_t scalar) { return uf::vector::multiply_(*this, scalar); } \
	FORCE_INLINE Vector<T,N>& operator/=(type_t scalar) { return uf::vector::divide_(*this, scalar); } \
	FORCE_INLINE bool operator==(const Vector<T,N>& rhs) const { return uf::vector::equals(*this, rhs); } \
	FORCE_INLINE bool operator!=(const Vector<T,N>& rhs) const { return uf::vector::notEquals(*this, rhs); } \
	FORCE_INLINE bool operator<(const Vector<T,N>& rhs) const { return uf::vector::less(*this, rhs); } \
	FORCE_INLINE bool operator<=(const Vector<T,N>& rhs) const { return uf::vector::lessEquals(*this, rhs); } \
	FORCE_INLINE bool operator>(const Vector<T,N>& rhs) const { return uf::vector::greater(*this, rhs); } \
	FORCE_INLINE bool operator>=(const Vector<T,N>& rhs) const { return uf::vector::greaterEquals(*this, rhs); } \
	FORCE_INLINE explicit operator bool() const { return uf::vector::notEquals(*this, Vector<T,N>{}); } \
	template<typename U, size_t M> FORCE_INLINE operator Vector<U,M>() const { return uf::vector::cast<U,M>(*this); }

#define DEFINE_VECTOR_EXT(T, N) \
	template<typename T> FORCE_INLINE pod::Vector<T,N> operator+( T scalar, const pod::Vector<T,N>& v ) { return uf::vector::add(scalar, v); }\
	template<typename T> FORCE_INLINE pod::Vector<T,N> operator-( T scalar, const pod::Vector<T,N>& v ) { return uf::vector::subtract(scalar, v); }\
	template<typename T> FORCE_INLINE pod::Vector<T,N> operator*( T scalar, const pod::Vector<T,N>& v ) { return uf::vector::multiply(scalar, v); }\
	template<typename T> FORCE_INLINE pod::Vector<T,N> operator/( T scalar, const pod::Vector<T,N>& v ) { return uf::vector::divide(scalar, v); }\

namespace pod {
	template<typename T, size_t N>
	struct Vector {
		union {
			T components[N];
		};

		DEFINE_VECTOR(T, N);
	};
	template<typename T>
	struct Vector<T,2> {
		union {
			struct { T x, y; };
			T components[2];
		};

		DEFINE_VECTOR(T, 2);
	};
	template<typename T>
	struct Vector<T,3> {
		union {
			struct { T x, y, z; };
			T components[3];
		};

		DEFINE_VECTOR(T, 3);
	};

	template<typename T>
	struct Vector<T,4> {
		union {
			struct { T x, y, z, w; };
			T components[4];
		};

		DEFINE_VECTOR(T, 4);
	};
}

// external functions

// scalar operator vector
template<typename T, size_t N> FORCE_INLINE pod::Vector<T,N> operator+( T scalar, const pod::Vector<T,N>& v ) { return uf::vector::add(scalar, v); }
template<typename T, size_t N> FORCE_INLINE pod::Vector<T,N> operator-( T scalar, const pod::Vector<T,N>& v ) { return uf::vector::subtract(scalar, v); }
template<typename T, size_t N> FORCE_INLINE pod::Vector<T,N> operator*( T scalar, const pod::Vector<T,N>& v ) { return uf::vector::multiply(scalar, v); }
template<typename T, size_t N> FORCE_INLINE pod::Vector<T,N> operator/( T scalar, const pod::Vector<T,N>& v ) { return uf::vector::divide(scalar, v); }

DEFINE_VECTOR_EXT(T, 2);
DEFINE_VECTOR_EXT(T, 3);
DEFINE_VECTOR_EXT(T, 4);

// stringify
namespace uf {
	namespace string {
		template<typename T, size_t N> FORCE_INLINE uf::stl::string toString( const pod::Vector<T,N>& v );
	}
}
// jsonify
namespace ext {
	namespace json {
		template<typename T, size_t N> FORCE_INLINE ext::json::Value encode( const pod::Vector<T,N>& v );
		template<typename T, size_t N> FORCE_INLINE pod::Vector<T,N>& decode( const ext::json::Value&, pod::Vector<T,N>& );
		template<typename T, size_t N> FORCE_INLINE pod::Vector<T,N> decode( const ext::json::Value&, const pod::Vector<T,N>& = {} );
	}
}
// hash
namespace std {
	template<typename T, size_t N>
	struct hash<pod::Vector<T,N>> {
		FORCE_INLINE size_t operator()(const pod::Vector<T,N>& v) const;
	};
}

#include "vector/vector.inl"