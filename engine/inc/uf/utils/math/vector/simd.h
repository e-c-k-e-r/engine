#pragma once

#include <uf/config.h>

#if UF_HALF_FLOATS
	#include <stdfloat>
#endif

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
		class /**UF_API**/ alignas(16) value {
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

			inline value operator+( const value& y );
			inline value operator-( const value& y );
			inline value operator*( const value& y );
			inline value operator/( const value& y );
			inline value& operator=(const value_type& rhs);
			inline value& operator=(const value& rhs);
			inline value& operator=(const pod::Vector4f& rhs);
			inline operator value_type() const;
			
			template<size_t N> inline operator pod::Vector<T,N>() const;
		};

		inline value<float> /**UF_API**/ load( const float* );
		inline void /**UF_API**/ store( value<float>, float* );
		inline value<float> /**UF_API**/ set( float );
		inline value<float> /**UF_API**/ set( float, float, float, float );
		inline value<float> /**UF_API**/ add( value<float>, value<float> );
		inline value<float> /**UF_API**/ sub( value<float>, value<float> );
		inline value<float> /**UF_API**/ mul( value<float>, value<float> );
		inline value<float> /**UF_API**/ div( value<float>, value<float> );
		inline value<float> /**UF_API**/ min( value<float>, value<float> );
		inline value<float> /**UF_API**/ max( value<float>, value<float> );
		inline value<float> /**UF_API**/ sqrt( value<float> );
	//	inline value<float> /**UF_API**/ hadd( value<float>, value<float> );
		inline float /**UF_API**/ dot( value<float>, value<float> );
		template<size_t N=4> inline pod::Vector<float,N> vector( const value<float> );

		inline value<int32_t> /**UF_API**/ load( const int32_t* );
		inline void /**UF_API**/ store( value<int32_t>, int32_t* );
		inline value<int32_t> /**UF_API**/ set( int32_t );
		inline value<int32_t> /**UF_API**/ set( int32_t, int32_t, int32_t, int32_t );
		inline value<int32_t> /**UF_API**/ add( value<int32_t>, value<int32_t> );
		inline value<int32_t> /**UF_API**/ sub( value<int32_t>, value<int32_t> );
		inline value<int32_t> /**UF_API**/ mul( value<int32_t>, value<int32_t> );
		inline value<int32_t> /**UF_API**/ div( value<int32_t>, value<int32_t> );
		inline value<int32_t> /**UF_API**/ min( value<int32_t>, value<int32_t> );
		inline value<int32_t> /**UF_API**/ max( value<int32_t>, value<int32_t> );
		inline value<int32_t> /**UF_API**/ sqrt( value<int32_t> );
	//	inline value<int32_t> /**UF_API**/ hadd( value<int32_t>, value<int32_t> );
		inline int32_t /**UF_API**/ dot( value<int32_t>, value<int32_t> );
		template<size_t N=4> inline pod::Vector<int32_t,N> vector( const value<int32_t> );

		inline value<uint> /**UF_API**/ load( const uint* );
		inline void /**UF_API**/ store( value<uint>, uint* );
		inline value<uint> /**UF_API**/ set( uint );
		inline value<uint> /**UF_API**/ set( uint, uint, uint, uint );
		inline value<uint> /**UF_API**/ add( value<uint>, value<uint> );
		inline value<uint> /**UF_API**/ sub( value<uint>, value<uint> );
		inline value<uint> /**UF_API**/ mul( value<uint>, value<uint> );
		inline value<uint> /**UF_API**/ div( value<uint>, value<uint> );
		inline value<uint> /**UF_API**/ min( value<uint>, value<uint> );
		inline value<uint> /**UF_API**/ max( value<uint>, value<uint> );
		inline value<uint> /**UF_API**/ sqrt( value<uint> );
	//	inline value<uint> /**UF_API**/ hadd( value<uint>, value<uint> );
		inline uint /**UF_API**/ dot( value<uint>, value<uint> );
		template<size_t N=4> inline pod::Vector<uint,N> vector( const value<uint> );

	// these are effectively NOPs
	#if UF_USE_FLOAT16
		inline value<std::float16_t> /**UF_API**/ load( const std::float16_t* ) { return {}; }
		inline void /**UF_API**/ store( value<std::float16_t>, std::float16_t* ) { return; }
		inline value<std::float16_t> /**UF_API**/ set( std::float16_t ) { return {}; }
		inline value<std::float16_t> /**UF_API**/ set( std::float16_t, std::float16_t, std::float16_t, std::float16_t ) { return {}; }
		inline value<std::float16_t> /**UF_API**/ add( value<std::float16_t>, value<std::float16_t> ) { return {}; }
		inline value<std::float16_t> /**UF_API**/ sub( value<std::float16_t>, value<std::float16_t> ) { return {}; }
		inline value<std::float16_t> /**UF_API**/ mul( value<std::float16_t>, value<std::float16_t> ) { return {}; }
		inline value<std::float16_t> /**UF_API**/ div( value<std::float16_t>, value<std::float16_t> ) { return {}; }
		inline value<std::float16_t> /**UF_API**/ min( value<std::float16_t>, value<std::float16_t> ) { return {}; }
		inline value<std::float16_t> /**UF_API**/ max( value<std::float16_t>, value<std::float16_t> ) { return {}; }
		inline value<std::float16_t> /**UF_API**/ sqrt( value<std::float16_t> ) { return {}; }
	//	inline value<std::float16_t> /**UF_API**/ hadd( value<std::float16_t>, value<std::float16_t> ) { return {}; }
		inline std::float16_t /**UF_API**/ dot( value<std::float16_t>, value<std::float16_t> ) { return {}; }
		template<size_t N=4> inline pod::Vector<std::float16_t,N> vector( const value<std::float16_t> ) { return {}; }
	#endif
	#if UF_USE_BFLOAT16
		inline value<std::bfloat16_t> /**UF_API**/ load( const std::bfloat16_t* ) { return {}; }
		inline void /**UF_API**/ store( value<std::bfloat16_t>, std::bfloat16_t* ) { return; }
		inline value<std::bfloat16_t> /**UF_API**/ set( std::bfloat16_t ) { return {}; }
		inline value<std::bfloat16_t> /**UF_API**/ set( std::bfloat16_t, std::bfloat16_t, std::bfloat16_t, std::bfloat16_t ) { return {}; }
		inline value<std::bfloat16_t> /**UF_API**/ add( value<std::bfloat16_t>, value<std::bfloat16_t> ) { return {}; }
		inline value<std::bfloat16_t> /**UF_API**/ sub( value<std::bfloat16_t>, value<std::bfloat16_t> ) { return {}; }
		inline value<std::bfloat16_t> /**UF_API**/ mul( value<std::bfloat16_t>, value<std::bfloat16_t> ) { return {}; }
		inline value<std::bfloat16_t> /**UF_API**/ div( value<std::bfloat16_t>, value<std::bfloat16_t> ) { return {}; }
		inline value<std::bfloat16_t> /**UF_API**/ min( value<std::bfloat16_t>, value<std::bfloat16_t> ) { return {}; }
		inline value<std::bfloat16_t> /**UF_API**/ max( value<std::bfloat16_t>, value<std::bfloat16_t> ) { return {}; }
		inline value<std::bfloat16_t> /**UF_API**/ sqrt( value<std::bfloat16_t> ) { return {}; }
	//	inline value<std::bfloat16_t> /**UF_API**/ hadd( value<std::bfloat16_t>, value<std::bfloat16_t> ) { return {}; }
		inline std::bfloat16_t /**UF_API**/ dot( value<std::bfloat16_t>, value<std::bfloat16_t> ) { return {}; }
		template<size_t N=4> inline pod::Vector<std::bfloat16_t,N> vector( const value<std::bfloat16_t> ) { return {}; }


	#endif
	}

}

#include "simd.inl"