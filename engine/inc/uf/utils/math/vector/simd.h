#pragma once

#include <uf/config.h>

#if UF_HALF_FLOATS
	#include <stdfloat>
#endif

#define DEFINE_SIMD(T)\
	FORCE_INLINE vector<T> /*UF_API*/ load( const T* );\
	FORCE_INLINE void /*UF_API*/ store( vector<T>, T* );\
	FORCE_INLINE vector<T> /*UF_API*/ set( T );\
	FORCE_INLINE vector<T> /*UF_API*/ set( T, T, T, T );\
	FORCE_INLINE vector<T> /*UF_API*/ add( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ sub( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ mul( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ div( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ min( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ max( vector<T>, vector<T> );\
	FORCE_INLINE bool /*UF_API*/ all( vector<T> );\
	FORCE_INLINE bool /*UF_API*/ any( vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ less( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ lessEquals( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ greater( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ greaterEquals( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ equals( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ notEquals( vector<T>, vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ sqrt( vector<T> );\
	FORCE_INLINE vector<T> /*UF_API*/ hadd( vector<T>, vector<T> );\
	FORCE_INLINE T /*UF_API*/ dot( vector<T>, vector<T> );\
	template<size_t N = 4> FORCE_INLINE pod::Vector<T,N> cast( const vector<T> );\

namespace uf {
	namespace simd {
		template<typename T>
		struct UF_API traits {
			static constexpr size_t size = 4;
			typedef T type;
			typedef __m128 value;
			typedef pod::Vector<T,size> vector;
		};

		template<>
		struct UF_API traits<int32_t> {
			static constexpr size_t size = 4;
			typedef int32_t type;
			typedef __m128i value;
			typedef pod::Vector<int32_t,size> vector;
		};
		template<>
		struct UF_API traits<uint32_t> {
			static constexpr size_t size = 4;
			typedef uint32_t type;
			typedef __m128i value;
			typedef pod::Vector<uint32_t,size> vector;
		};
		template<>
		struct UF_API traits<float> {
			static constexpr size_t size = 4;
			typedef float type;
			typedef __m128 value;
			typedef pod::Vector<float,size> vector;
		};

		template<typename T>
		class /*UF_API*/ alignas(16) vector {
		public:
		//	__m128 m;
			typedef typename traits<T>::value value_type;
			value_type m;
			FORCE_INLINE vector();
			FORCE_INLINE vector(const T* f);
			FORCE_INLINE vector(T f);
			FORCE_INLINE vector(T f0, T f1, T f2, T f3);
			FORCE_INLINE vector(bool f0, bool f1, bool f2, bool f3);
			FORCE_INLINE vector(const value_type& rhs);
			FORCE_INLINE vector(const vector& rhs);

			FORCE_INLINE vector(const pod::Vector<T,1>& rhs);
			FORCE_INLINE vector(const pod::Vector<T,2>& rhs);
			FORCE_INLINE vector(const pod::Vector<T,3>& rhs);
			FORCE_INLINE vector(const pod::Vector<T,4>& rhs);

			FORCE_INLINE vector operator+( const vector& rhs );
			FORCE_INLINE vector operator-( const vector& rhs );
			FORCE_INLINE vector operator*( const vector& rhs );
			FORCE_INLINE vector operator/( const vector& rhs );

			FORCE_INLINE vector operator<( const vector& rhs );
			FORCE_INLINE vector operator<=( const vector& rhs );
			FORCE_INLINE vector operator>( const vector& rhs );
			FORCE_INLINE vector operator>=( const vector& rhs );
			FORCE_INLINE vector operator==( const vector& rhs );
			FORCE_INLINE vector operator!=( const vector& rhs );
			
			FORCE_INLINE vector& operator=(const value_type& rhs);
			FORCE_INLINE vector& operator=(const vector& rhs);
			FORCE_INLINE vector& operator=(const pod::Vector<T,4>& rhs);

			FORCE_INLINE operator value_type() const;
			
			template<size_t N> FORCE_INLINE operator pod::Vector<T,N>() const;
		};

		DEFINE_SIMD(float);
		DEFINE_SIMD(int32_t);
		DEFINE_SIMD(uint32_t);

	// these are effectively NOPs
	/*
	#if UF_USE_FLOAT16
		DEFINE_SIMD(std::float16_t)
	#endif
	#if UF_USE_BFLOAT16
		DEFINE_SIMD(std::bfloat16_t)
	#endif
	*/

		// specializations
		FORCE_INLINE vector<float> /*UF_API*/ set_f( bool, bool, bool, bool );
		FORCE_INLINE vector<int32_t> /*UF_API*/ set_i( bool, bool, bool, bool );
		FORCE_INLINE vector<uint32_t> /*UF_API*/ set_ui( bool, bool, bool, bool );

		FORCE_INLINE vector<float> /*UF_API*/ cross( vector<float> x, vector<float> y );
		FORCE_INLINE vector<float> /*UF_API*/ normalize( vector<float> x );
		FORCE_INLINE vector<float> /*UF_API*/ normalize_fast( vector<float> x );
	}
}

#include "simd.inl"