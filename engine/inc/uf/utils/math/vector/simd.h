#pragma once

#include <uf/config.h>

#if UF_HALF_FLOATS
	#include <stdfloat>
#endif

#define DEFINE_SIMD(T)\
	inline value<T> /*UF_API*/ load( const T* );\
	inline void /*UF_API*/ store( value<T>, T* );\
	inline value<T> /*UF_API*/ set( T );\
	inline value<T> /*UF_API*/ set( T, T, T, T );\
	inline value<T> /*UF_API*/ add( value<T>, value<T> );\
	inline value<T> /*UF_API*/ sub( value<T>, value<T> );\
	inline value<T> /*UF_API*/ mul( value<T>, value<T> );\
	inline value<T> /*UF_API*/ div( value<T>, value<T> );\
	inline value<T> /*UF_API*/ min( value<T>, value<T> );\
	inline value<T> /*UF_API*/ max( value<T>, value<T> );\
	inline bool /*UF_API*/ all( value<T> );\
	inline bool /*UF_API*/ any( value<T> );\
	inline value<T> /*UF_API*/ less( value<T>, value<T> );\
	inline value<T> /*UF_API*/ lessEquals( value<T>, value<T> );\
	inline value<T> /*UF_API*/ greater( value<T>, value<T> );\
	inline value<T> /*UF_API*/ greaterEquals( value<T>, value<T> );\
	inline value<T> /*UF_API*/ equals( value<T>, value<T> );\
	inline value<T> /*UF_API*/ notEquals( value<T>, value<T> );\
	inline value<T> /*UF_API*/ sqrt( value<T> );\
	inline value<T> /*UF_API*/ hadd( value<T>, value<T> );\
	inline T /*UF_API*/ dot( value<T>, value<T> );\
	template<size_t N = 4> inline pod::Vector<T,N> vector( const value<T> );\

namespace uf {
	namespace simd {
		template<typename T>
		struct UF_API traits {
			static const size_t size = 4;
			typedef T type;
			typedef __m128 value;
			typedef pod::Vector<T,size> vector;
		};

		template<>
		struct UF_API traits<int32_t> {
			static const size_t size = 4;
			typedef int32_t type;
			typedef __m128i value;
			typedef pod::Vector<int32_t,4> vector;
		};
		template<>
		struct UF_API traits<uint32_t> {
			static const size_t size = 4;
			typedef uint32_t type;
			typedef __m128i value;
			typedef pod::Vector<uint32_t,4> vector;
		};
		template<>
		struct UF_API traits<float> {
			static const size_t size = 4;
			typedef float type;
			typedef __m128 value;
			typedef pod::Vector<float,4> vector;
		};

		template<typename T>
		class /*UF_API*/ alignas(16) value {
		private:
		//	__m128 m_value;
			typedef typename traits<T>::value value_type;
			value_type m_value;
		public:
			inline value();
			inline value(const T* f);
			inline value(T f);
			inline value(T f0, T f1, T f2, T f3);
			inline value(const value_type& rhs);
			inline value(const value& rhs);

			inline value(const pod::Vector<T,1>& rhs);
			inline value(const pod::Vector<T,2>& rhs);
			inline value(const pod::Vector<T,3>& rhs);
			inline value(const pod::Vector<T,4>& rhs);

			inline value operator+( const value& rhs );
			inline value operator-( const value& rhs );
			inline value operator*( const value& rhs );
			inline value operator/( const value& rhs );

			inline value operator<( const value& rhs );
			inline value operator<=( const value& rhs );
			inline value operator>( const value& rhs );
			inline value operator>=( const value& rhs );
			inline value operator==( const value& rhs );
			inline value operator!=( const value& rhs );
			
			inline value& operator=(const value_type& rhs);
			inline value& operator=(const value& rhs);
			inline value& operator=(const pod::Vector<T,4>& rhs);

			inline operator value_type() const;
			
			template<size_t N> inline operator pod::Vector<T,N>() const;
		};

		DEFINE_SIMD(float);
		DEFINE_SIMD(int32_t);
		DEFINE_SIMD(uint32_t);

	// these are effectively NOPs
	#if UF_USE_FLOAT16
		DEFINE_SIMD(std::float16_t)
	#endif
	#if UF_USE_BFLOAT16
		DEFINE_SIMD(std::bfloat16_t)
	#endif
	}
}

#include "simd.inl"