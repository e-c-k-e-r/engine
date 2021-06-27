#pragma once

#include <uf/config.h>

#include "math.h"

#include <sstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <stdint.h>

#include <uf/ext/json/json.h>
#include <uf/utils/math/angle.h>

namespace pod {
	// Simple vectors (designed [to store in arrays] with minimal headaches)
	template<typename T = pod::Math::num_t, std::size_t N = 3>
	struct /*UF_API*/ Vector {
	// 	n-dimensional/unspecialized vector access
		T components[N];
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = N;
	// 	Overload access
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		Vector<T,N> operator()() const; 								// 	Negation
		Vector<T,N> operator-() const; 								// 	Negation
		Vector<T,N> operator+( const Vector<T,N>& vector ) const; 	// 	Addition between two vectors
		Vector<T,N> operator-( const Vector<T,N>& vector ) const; 	// 	Subtraction between two vectors
		Vector<T,N> operator*( const Vector<T,N>& vector ) const; 	// 	Multiplication between two vectors
		Vector<T,N> operator/( const Vector<T,N>& vector ) const; 	// 	Division between two vectors
		Vector<T,N> operator*( T scalar ) const; 					// 	Multiplication with scalar
		Vector<T,N> operator/( T scalar ) const; 					// 	Division with scalar
		Vector<T,N>& operator +=( const Vector<T,N>& vector ); 		// 	Addition set between two vectors
		Vector<T,N>& operator -=( const Vector<T,N>& vector ); 		// 	Subtraction set between two vectors
		Vector<T,N>& operator *=( const Vector<T,N>& vector ); 		// 	Multiplication set between two vectors
		Vector<T,N>& operator /=( const Vector<T,N>& vector ); 		// 	Division set between two vectors
		Vector<T,N>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		Vector<T,N>& operator /=( T scalar ); 						// 	Division set with scalar
		bool operator==( const Vector<T,N>& vector ) const; 		// 	Equality check between two vectors (equals)
		bool operator!=( const Vector<T,N>& vector ) const; 		// 	Equality check between two vectors (not equals)
		bool operator<( const Vector<T,N>& vector ) const; 			// 	Equality check between two vectors (less than)
		bool operator<=( const Vector<T,N>& vector ) const; 		// 	Equality check between two vectors (less than or equals)
		bool operator>( const Vector<T,N>& vector ) const; 			// 	Equality check between two vectors (greater than)
		bool operator>=( const Vector<T,N>& vector ) const; 		// 	Equality check between two vectors (greater than or equals)

		template<typename U, size_t M> Vector<T,N>& operator=( const Vector<U,M>& vector );
		template<typename U, size_t M> operator Vector<U,M>();
		inline operator bool() const;
	};
	template<typename T = float> using Vector1t = Vector<T,1>;
	typedef Vector1t<pod::Math::num_t> Vector1;
	typedef Vector1t<int32_t> Vector1i;
	typedef Vector1t<uint32_t> Vector1ui;

	typedef Vector1t<long> Vector1l;
	typedef Vector1t<float> Vector1f;
	typedef Vector1t<double> Vector1d;

	template<typename T = float> using Vector2t = Vector<T,2>;
	typedef Vector2t<pod::Math::num_t> Vector2;
	typedef Vector2t<int32_t> Vector2i;
	typedef Vector2t<uint32_t> Vector2ui;

	typedef Vector2t<long> Vector2l;
	typedef Vector2t<float> Vector2f;
	typedef Vector2t<double> Vector2d;

	template<typename T = float> using Vector3t = Vector<T,3>;
	typedef Vector3t<pod::Math::num_t> Vector3;
	typedef Vector3t<int32_t> Vector3i;
	typedef Vector3t<uint32_t> Vector3ui;
	typedef Vector3t<uint8_t> ColorRGB;

	typedef Vector3t<long> Vector3l;
	typedef Vector3t<float> Vector3f;
	typedef Vector3t<double> Vector3d;

	template<typename T = float> using Vector4t = Vector<T,4>;
	typedef Vector4t<pod::Math::num_t> Vector4;
	typedef Vector4t<int32_t> Vector4i;
	typedef Vector4t<uint32_t> Vector4ui;
	typedef Vector4t<uint8_t> ColorRgba;

	typedef Vector4t<long> Vector4l;
	typedef Vector4t<float> Vector4f;
	typedef Vector4t<double> Vector4d;
}

// 	POD vector accessing/manipulation
namespace uf {
	namespace vector {
		template<typename T> pod::Vector1t<T> /*UF_API*/ create( T x );
		template<typename T> pod::Vector2t<T> /*UF_API*/ create( T x, T y );
		template<typename T> pod::Vector3t<T> /*UF_API*/ create( T x, T y, T z );
		template<typename T> pod::Vector4t<T> /*UF_API*/ create( T x, T y, T z, T w );
		template<typename T, size_t N> pod::Vector<T, N> /*UF_API*/ copy( const pod::Vector<T, N>& = {});
		template<typename T, size_t N, typename U> pod::Vector<T, N> /*UF_API*/ cast( const U& from );
	// 	Equality checking
		template<typename T> int /*UF_API*/ compareTo( const T& left, const T& right ); 					// 	Equality check between two vectors (less than)
		template<typename T> bool /*UF_API*/ equals( const T& left, const T& right ); 						// 	Equality check between two vectors (equals)
	// 	Basic arithmetic
		template<typename T> T /*UF_API*/ add( const T& left, const T& right );								// 	Adds two vectors of same type and size together
		template<typename T> T /*UF_API*/ subtract( const T& left, const T& right );						// 	Subtracts two vectors of same type and size together
		template<typename T> T /*UF_API*/ multiply( const T& left, const T& right );						// 	Multiplies two vectors of same type and size together
		template<typename T> T /*UF_API*/ multiply( const T& vector, const typename T::type_t& scalar );	// 	Multiplies this vector by a scalar
		template<typename T> T /*UF_API*/ divide( const T& left, const T& right );							// 	Divides two vectors of same type and size together
		template<typename T> T /*UF_API*/ divide( const T& left, const typename T::type_t& scalar );		// 	Divides this vector by a scalar
		template<typename T> typename T::type_t /*UF_API*/ sum( const T& vector );							// 	Compute the sum of all components 
		template<typename T> typename T::type_t /*UF_API*/ product( const T& vector );						// 	Compute the product of all components 
		template<typename T> T /*UF_API*/ negate( const T& vector );										// 	Flip sign of all components
	// 	Writes to first value
		template<typename T> T& /*UF_API*/ add( T& left, const T& right );									// 	Adds two vectors of same type and size together
		template<typename T> T& /*UF_API*/ subtract( T& left, const T& right );								// 	Subtracts two vectors of same type and size together
		template<typename T> T& /*UF_API*/ multiply( T& left, const T& right );								// 	Multiplies two vectors of same type and size together
		template<typename T> T& /*UF_API*/ multiply( T& vector, const typename T::type_t& scalar );			// 	Multiplies this vector by a scalar
		template<typename T> T& /*UF_API*/ divide( T& left, const T& right );								// 	Divides two vectors of same type and size together
		template<typename T> T& /*UF_API*/ divide( T& left, const typename T::type_t& scalar );				// 	Divides this vector by a scalar
		template<typename T> T& /*UF_API*/ negate( T& vector );												// 	Flip sign of all components
		template<typename T> T& /*UF_API*/ normalize( T& vector ); 											// 	Normalizes a vector
	// 	Complex arithmetic
		template<typename T> typename T::type_t /*UF_API*/ dot( const T& left, const T& right ); 			// 	Compute the dot product between two vectors
		template<typename T> pod::Angle /*UF_API*/ angle( const T& a, const T& b ); 						// 	Compute the angle between two vectors
		template<typename T> T /*UF_API*/ cross( const T& a, const T& b ); 									// 	Compute the angle between two vectors
		
		template<typename T> T /*UF_API*/ lerp( const T& from, const T& to, double, bool = true  ); 		// 	Linearly interpolate between two vectors
		template<typename T> T /*UF_API*/ lerp( const T& from, const T& to, const T&, bool = true  ); 		// 	Linearly interpolate between two vectors

		template<typename T> T /*UF_API*/ slerp( const T& from, const T& to, double, bool = false); 		// 	Spherically interpolate between two vectors
		template<typename T> T /*UF_API*/ mix( const T& from, const T& to, double, bool = false ); 			// 	
		
		template<typename T> typename T::type_t /*UF_API*/ distanceSquared( const T& a, const T& b ); 		// 	Compute the distance between two vectors (doesn't sqrt)
		template<typename T> typename T::type_t /*UF_API*/ distance( const T& a, const T& b ); 				// 	Compute the distance between two vectors
		template<typename T> typename T::type_t /*UF_API*/ magnitude( const T& vector ); 					// 	Gets the magnitude of the vector
		template<typename T> typename T::type_t /*UF_API*/ norm( const T& vector ); 						// 	Compute the norm of the vector
		template<typename T> T /*UF_API*/ normalize( const T& vector ); 									// 	Normalizes a vector
		template<typename T> void /*UF_API*/ orthonormalize( T& x, T& y ); 									// 	Normalizes a vector
		template<typename T> T /*UF_API*/ orthonormalize( const T& x, const T& y ); 						// 	Normalizes a vector
		
		template<typename T> std::string /*UF_API*/ toString( const T& vector ); 							// 	Parses a vector as a string
		template<typename T, size_t N> ext::json::Value encode( const pod::Vector<T,N>& v, const ext::json::EncodingSettings& = {} ); 				// 	Parses a vector into a JSON value
		
		template<typename T, size_t N> pod::Vector<T,N>& decode( const ext::json::Value& v, pod::Vector<T,N>& ); 							// 	Parses a JSON value into a vector
		template<typename T, size_t N> pod::Vector<T,N> decode( const ext::json::Value& v, const pod::Vector<T,N>& = {} ); 				// 	Parses a JSON value into a vector
	}
}


namespace uf {
	// Provides operations for POD vector
	template<typename T = pod::Math::num_t, std::size_t N = 3> 
	class /*UF_API*/ Vector {
	public:
	// 	Easily access POD's type
		typedef pod::Vector<T,N> pod_t;
	// 	Replicate POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = N;
	protected:
	// 	POD storage
		Vector<T,N>::pod_t m_pod;
	public:
	// 	C-tor
		Vector(); 																		// initializes POD to 'def'
		Vector(T def); 																	// initializes POD to 'def'
		Vector(const Vector<T,N-1>& v, T w);
		Vector(const Vector<T,N>::pod_t& pod); 											// copies POD altogether
		Vector(const T components[N]); 													// copies data into POD from 'components' (typed as C array)
		Vector(const std::vector<T>& components); 										// copies data into POD from 'components' (typed as std::vector<T>)
	// 	D-tor
		// Unneccesary
	// 	POD access
		Vector<T,N>::pod_t& data(); 													// 	Returns a reference of POD
		const Vector<T,N>::pod_t& data() const; 										// 	Returns a const-reference of POD
	// 	Alternative POD access
		T* get();																		// 	Returns a pointer to the entire array
		const T* get() const; 															// 	Returns a const-pointer to the entire array
		T& getComponent( std::size_t i );												// 	Returns a reference to a single element
		const T& getComponent( std::size_t i ) const; 									// 	Returns a const-reference to a single element
	// 	POD manipulation
		T* set(const T components[N]);													// 	Sets the entire array
		T& setComponent( std::size_t i, const T& value );								// 	Sets a single element
	// 	Validation
		bool isValid() const; 															// 	Checks if all components are valid (non NaN, inf, etc.)
	// 	Basic arithmetic
		inline uf::Vector<T,N>& add( const Vector<T, N>& vector );						// 	Adds two vectors of same type and size together
		inline uf::Vector<T,N>& subtract( const Vector<T, N>& vector );					// 	Subtracts two vectors of same type and size together
		inline uf::Vector<T,N>& multiply( const Vector<T, N>& vector );					// 	Multiplies two vectors of same type and size together
		inline uf::Vector<T,N>& multiply( T scalar );									// 	Multiplies this vector by a scalar
		inline uf::Vector<T,N>& divide( const Vector<T, N>& vector );					// 	Divides two vectors of same type and size together
		inline uf::Vector<T,N>& divide( T scalar );										// 	Divides this vector by a scalar
		inline T sum() const;															// 	Compute the sum of all components 
		inline T product() const;														// 	Compute the product of all components 
		inline uf::Vector<T,N>& negate();												// 	Flip sign of all components
	// 	Complex arithmetic
		inline T dot( const Vector<T,N> right ) const; 									// 	Compute the dot product between two vectors
		inline pod::Angle angle( const Vector<T,N>& b ) const; 							// 	Compute the angle between two vectors
		
		inline uf::Vector<T,N> lerp( const Vector<T,N> to, double delta ) const; 		// 	Linearly interpolate between two vectors
		inline uf::Vector<T,N> slerp( const Vector<T,N> to, double delta ) const; 		// 	Spherically interpolate between two vectors
		
		inline T distanceSquared( const Vector<T,N> b ) const; 							// 	Compute the distance between two vectors (doesn't sqrt)
		inline T distance( const Vector<T,N> b ) const; 								// 	Compute the distance between two vectors
		inline T magnitude() const; 													// 	Gets the magnitude of the vector
		inline T norm() const; 															// 	Compute the norm of the vector
		
		inline uf::Vector<T,N>& normalize(); 											// 	Normalizes a vector
		uf::Vector<T,N> getNormalized() const; 											// 	Return a normalized vector
		inline std::string toString() const;
	// 	Overloaded ops
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,N> operator-() const; 								// 	Negation
		inline Vector<T,N> operator+( const Vector<T,N>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,N> operator-( const Vector<T,N>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,N> operator*( const Vector<T,N>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,N> operator/( const Vector<T,N>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,N> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,N> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,N>& operator +=( const Vector<T,N>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,N>& operator -=( const Vector<T,N>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,N>& operator *=( const Vector<T,N>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,N>& operator /=( const Vector<T,N>& vector ); 		// 	Division set between two vectors
		inline Vector<T,N>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,N>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,N>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,N>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,N>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,N>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,N>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,N>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		
		inline operator pod_t&() { return this->m_pod; }
		inline operator const pod_t&() const { return this->m_pod; }
	};
	template<typename T = float> using Vector1t = Vector<T,1>;
	typedef Vector1t<pod::Math::num_t> Vector1;
	typedef Vector1t<int32_t> Vector1i;
	typedef Vector1t<uint32_t> Vector1ui;

	typedef Vector1t<long> Vector1l;
	typedef Vector1t<float> Vector1f;
	typedef Vector1t<double> Vector1d;
	
	template<typename T = float> using Vector2t = Vector<T,2>;
	typedef Vector2t<pod::Math::num_t> Vector2;
	typedef Vector2t<int32_t> Vector2i;
	typedef Vector2t<uint32_t> Vector2ui;

	typedef Vector2t<long> Vector2l;
	typedef Vector2t<float> Vector2f;
	typedef Vector2t<double> Vector2d;

	template<typename T = float> using Vector3t = Vector<T,3>;
	typedef Vector3t<pod::Math::num_t> Vector3;
	typedef Vector3t<int32_t> Vector3i;
	typedef Vector3t<uint32_t> Vector3ui;
	typedef Vector3t<uint8_t> ColorRGB;

	typedef Vector3t<long> Vector3l;
	typedef Vector3t<float> Vector3f;
	typedef Vector3t<double> Vector3d;

	template<typename T = float> using Vector4t = Vector<T,4>;
	typedef Vector4t<pod::Math::num_t> Vector4;
	typedef Vector4t<int32_t> Vector4i;
	typedef Vector4t<uint32_t> Vector4ui;
	typedef Vector4t<uint8_t> ColorRgba;

	typedef Vector4t<long> Vector4l;
	typedef Vector4t<float> Vector4f;
	typedef Vector4t<double> Vector4d;
}

//#include <uf/utils/string/ext.h>
#include <sstream>
namespace uf {
	namespace string {
		template<typename T, size_t N>
		std::string toString( const pod::Vector<T,N>& v );
	}
}
namespace ext {
	namespace json {
		template<typename T, size_t N> ext::json::Value encode( const pod::Vector<T,N>& v );
		template<typename T, size_t N> pod::Vector<T,N>& decode( const ext::json::Value&, pod::Vector<T,N>& );
		template<typename T, size_t N> pod::Vector<T,N> decode( const ext::json::Value&, const pod::Vector<T,N>& = {} );
	}
}

namespace pod {
	template<typename T>
	struct /*UF_API*/ Vector<T,1> {			
	// 	XY access
		T x;
	// 	n-dimensional/unspecialized vector access
	//	T* components = (T*) this;
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 1;
	// 	Overload access
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,1> operator()() const; 								// 	Creation
		inline Vector<T,1> operator-() const; 								// 	Negation
		inline Vector<T,1> operator+( const Vector<T,1>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,1> operator-( const Vector<T,1>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,1> operator*( const Vector<T,1>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,1> operator/( const Vector<T,1>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,1> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,1> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,1>& operator +=( const Vector<T,1>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,1>& operator -=( const Vector<T,1>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,1>& operator *=( const Vector<T,1>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,1>& operator /=( const Vector<T,1>& vector ); 		// 	Division set between two vectors
		inline Vector<T,1>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,1>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		template<typename U, size_t M> Vector<T,1>& operator=( const Vector<U,M>& vector );
		template<typename U, size_t M> operator Vector<U,M>();
		inline operator bool() const;
	#if 0
	#if UF_USE_SIMD
		Vector<T,1>& operator=( const __m128 );
		operator __m128() const;
	#endif
	#endif
	};
	template<typename T>
	struct /*UF_API*/ Vector<T,2> {			
	// 	XY access
		T x;
		T y;
	// 	n-dimensional/unspecialized vector access
	//	T* components = (T*) this;
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 2;
	// 	Overload access
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,2> operator()() const; 								// 	Creation
		inline Vector<T,2> operator-() const; 								// 	Negation
		inline Vector<T,2> operator+( const Vector<T,2>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,2> operator-( const Vector<T,2>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,2> operator*( const Vector<T,2>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,2> operator/( const Vector<T,2>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,2> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,2> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,2>& operator +=( const Vector<T,2>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,2>& operator -=( const Vector<T,2>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,2>& operator *=( const Vector<T,2>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,2>& operator /=( const Vector<T,2>& vector ); 		// 	Division set between two vectors
		inline Vector<T,2>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,2>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		template<typename U, size_t M> Vector<T,2>& operator=( const Vector<U,M>& vector );
		template<typename U, size_t M> operator Vector<U,M>();
		inline operator bool() const;
	#if 0
	#if UF_USE_SIMD
		Vector<T,2>& operator=( const __m128 );
		operator __m128() const;
	#endif
	#endif
	};
	template<typename T>
	struct /*UF_API*/ Vector<T,3> {
	// 	XYZ access
		T x;
		T y;
		T z;
	// 	n-dimensional/unspecialized vector access
	//	T* components = (T*) this;
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 3;
	// 	Overload access
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,3> operator()() const; 								// 	Creation
		inline Vector<T,3> operator-() const; 								// 	Negation
		inline Vector<T,3> operator+( const Vector<T,3>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,3> operator-( const Vector<T,3>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,3> operator*( const Vector<T,3>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,3> operator/( const Vector<T,3>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,3> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,3> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,3>& operator +=( const Vector<T,3>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,3>& operator -=( const Vector<T,3>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,3>& operator *=( const Vector<T,3>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,3>& operator /=( const Vector<T,3>& vector ); 		// 	Division set between two vectors
		inline Vector<T,3>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,3>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		
		template<typename U, size_t M> Vector<T,3>& operator=( const Vector<U,M>& vector );
		template<typename U, size_t M> operator Vector<U,M>();
		inline operator bool() const;
	#if 0
	#if UF_USE_SIMD
		Vector<T,3>& operator=( const __m128 );
		operator __m128() const;
	#endif
	#endif
	};
	template<typename T>
	struct /*UF_API*/ /*alignas(16)*/ Vector<T,4> {
	// 	XYZW access
		T x;
		T y;
		T z;
		T w;
	// 	n-dimensional/unspecialized vector access
	//	T* components = (T*) this;
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 4;
	// 	Overload access
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,4> operator()() const; 								// 	Creation
		inline Vector<T,4> operator-() const; 								// 	Negation
		inline Vector<T,4> operator+( const Vector<T,4>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,4> operator-( const Vector<T,4>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,4> operator*( const Vector<T,4>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,4> operator/( const Vector<T,4>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,4> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,4> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,4>& operator +=( const Vector<T,4>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,4>& operator -=( const Vector<T,4>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,4>& operator *=( const Vector<T,4>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,4>& operator /=( const Vector<T,4>& vector ); 		// 	Division set between two vectors
		inline Vector<T,4>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,4>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		
		template<typename U, size_t M> Vector<T,4>& operator=( const Vector<U,M>& vector );
		template<typename U, size_t M> operator Vector<U,M>();
		inline operator bool() const;
	#if 0
	#if UF_USE_SIMD
		Vector<T,4>& operator=( const __m128 );
		operator __m128() const;
	#endif
	#endif
	};
}

#include "vector/vector.inl"