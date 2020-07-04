#pragma once

#include <uf/config.h>

namespace pod {
	// Simple Pixels (designed [to store in arrays] with minimal headaches)
	template<typename T = pod::Math::num_t, std::size_t N = 4>
	struct UF_API Pixel {
	// 	n-dimensional/unspecialized Pixel access
		T components[N];
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = N;
	// 	Overload access
		// Accessing Pia subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Pixel<T,N> operator-() const; 							// 	Negation
		inline Pixel<T,N> operator*( const Pixel<T,N>& Pixel ) const; 	// 	Multiplication between two Pixels
		inline Pixel<T,N> operator/( const Pixel<T,N>& Pixel ) const; 	// 	Divison between two Pixels
		inline Pixel<T,N> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Pixel<T,N> operator/( T scalar ) const; 					// 	Divison with scalar
		inline Pixel<T,N>& operator *=( const Pixel<T,N>& Pixel ); 		// 	Multiplication set between two Pixels
		inline Pixel<T,N>& operator /=( const Pixel<T,N>& Pixel ); 		// 	Divison set between two Pixels
		inline Pixel<T,N>& operator *=( T scalar ); 					// 	Multiplication set with scalar
		inline Pixel<T,N>& operator /=( T scalar ); 					// 	Divison set with scalar
		inline bool operator==( const Pixel<T,N>& Pixel ) const; 		// 	Equality check between two Pixels (equals)
		inline bool operator!=( const Pixel<T,N>& Pixel ) const; 		// 	Equality check between two Pixels (not equals)
	};
	template<typename T>
	struct UF_API Pixel<T,3> {
	// 	XYZ access
		T r = 0;
		T g = 0;
		T b = 0;
	// 	n-dimensional/unspecialized Pixel access
		T* components = (T*) this;
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 3;
	// 	Overload access
		// Accessing Pia subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Pixel<T,3> operator-() const; 							// 	Negation
		inline Pixel<T,3> operator*( const Pixel<T,3>& Pixel ) const; 	// 	Multiplication between two Pixels
		inline Pixel<T,3> operator/( const Pixel<T,3>& Pixel ) const; 	// 	Divison between two Pixels
		inline Pixel<T,3> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Pixel<T,3> operator/( T scalar ) const; 					// 	Divison with scalar
		inline Pixel<T,3>& operator *=( const Pixel<T,3>& Pixel ); 		// 	Multiplication set between two Pixels
		inline Pixel<T,3>& operator /=( const Pixel<T,3>& Pixel ); 		// 	Divison set between two Pixels
		inline Pixel<T,3>& operator *=( T scalar ); 					// 	Multiplication set with scalar
		inline Pixel<T,3>& operator /=( T scalar ); 					// 	Divison set with scalar
		inline bool operator==( const Pixel<T,3>& Pixel ) const; 		// 	Equality check between two Pixels (equals)
		inline bool operator!=( const Pixel<T,3>& Pixel ) const; 		// 	Equality check between two Pixels (not equals)
	};
	template<typename T>
	struct UF_API Pixel<T,4> {
	// 	XYZW access
		T r = 0;
		T g = 0;
		T b = 0;
		T a = 0;
	// 	n-dimensional/unspecialized Pixel access
		T* components = (T*) this;
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 4;
	// 	Overload access
		// Accessing Pia subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Pixel<T,4> operator-() const; 							// 	Negation
		inline Pixel<T,4> operator*( const Pixel<T,4>& Pixel ) const; 	// 	Multiplication between two Pixels
		inline Pixel<T,4> operator/( const Pixel<T,4>& Pixel ) const; 	// 	Divison between two Pixels
		inline Pixel<T,4> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Pixel<T,4> operator/( T scalar ) const; 					// 	Divison with scalar
		inline Pixel<T,4>& operator *=( const Pixel<T,4>& Pixel ); 		// 	Multiplication set between two Pixels
		inline Pixel<T,4>& operator /=( const Pixel<T,4>& Pixel ); 		// 	Divison set between two Pixels
		inline Pixel<T,4>& operator *=( T scalar ); 					// 	Multiplication set with scalar
		inline Pixel<T,4>& operator /=( T scalar ); 					// 	Divison set with scalar
		inline bool operator==( const Pixel<T,4>& Pixel ) const; 		// 	Equality check between two Pixels (equals)
		inline bool operator!=( const Pixel<T,4>& Pixel ) const; 		// 	Equality check between two Pixels (not equals)
	};

	template<typename T = pod::Math::num_t> using Pixel3t = Pixel<T,3>;
	typedef Pixel<float> Pixel3f;
	typedef Pixel<double> Pixel3d;
	typedef Pixel<uint8_t> PixelRgb8;

	template<typename T = pod::Math::num_t> using Pixel4t = Pixel<T,4>;
	typedef Pixel<float> Pixel4f;
	typedef Pixel<double> Pixel4d;
	typedef Pixel<uint8_t> PixelRgba8;
}

// 	POD pixel accessing/manipulation
namespace uf {
	namespace pixel {
	// 	Equality checking
		template<typename T> std::size_t /*UF_API*/ compareTo( const T& left, const T& right ); 			// 	Equality check between two pixels (less than)
		template<typename T> bool /*UF_API*/ equals( const T& left, const T& right ); 						// 	Equality check between two pixels (equals)
	// 	Basic arithmetic
		template<typename T> T /*UF_API*/ multiply( const T& left, const T& right );						// 	Multiplies two pixels of same type and size together
		template<typename T> T /*UF_API*/ multiply( const T& pixel, const typename T::type_t& scalar );	// 	Multiplies this pixel by a scalar
		template<typename T> T /*UF_API*/ divide( const T& left, const T& right );							// 	Divides two pixels of same type and size together
		template<typename T> T /*UF_API*/ divide( const T& left, const typename T::type_t& scalar );		// 	Divides this pixel by a scalar
		template<typename T> T /*UF_API*/ negate( const T& pixel );										// 	Flip sign of all components
	// 	Writes to first value
		template<typename T> T& /*UF_API*/ multiply( T& left, const T& right );								// 	Multiplies two pixels of same type and size together
		template<typename T> T& /*UF_API*/ multiply( T& pixel, const typename T::type_t& scalar );			// 	Multiplies this pixel by a scalar
		template<typename T> T& /*UF_API*/ divide( T& left, const T& right );								// 	Divides two pixels of same type and size together
		template<typename T> T& /*UF_API*/ divide( T& left, const typename T::type_t& scalar );				// 	Divides this pixel by a scalar
		template<typename T> T& /*UF_API*/ negate( T& pixel );												// 	Flip sign of all components
	}
}
#include "pixel.inl"