#pragma once

#include <uf/config.h>

#include <sstream>
#include <uf/utils/memory/vector.h>
#include <cmath>
#include <stdint.h>

#include <uf/utils/math/angle.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>

#include "math.h"
namespace pod {
	// Simple quaterions (designed [to store in arrays] with minimal headaches)
	template<typename T = pod::Math::num_t> using Quaternion = Vector4t<T>;
}

namespace uf {
	namespace quaternion {
	// 	Equality checking
		template<typename T> std::size_t /*UF_API*/ compareTo( const T& left, const T& right ); 			// 	Equality check between two vectors (less than)
		template<typename T> bool /*UF_API*/ equals( const T& left, const T& right ); 						// 	Equality check between two vectors (equals)
	// 	Basic arithmetic
		template<typename T> T /*UF_API*/ multiply( const T& left, const T& right );						// 	Multiplies two vectors of same type and size together
		template<typename T = pod::Math::num_t> pod::Quaternion<T> /*UF_API*/ identity();					// 	Multiplies two vectors of same type and size together
		template<typename T> pod::Vector3t<T> /*UF_API*/ rotate( const pod::Quaternion<T>& left, const pod::Vector3t<T>& right );						// 	Multiplies two vectors of same type and size together
		template<typename T> pod::Vector4t<T> /*UF_API*/ rotate( const pod::Quaternion<T>& left, const pod::Vector4t<T>& right );						// 	Multiplies two vectors of same type and size together
		template<typename T> typename T::type_t /*UF_API*/ sum( const T& vector );							// 	Compute the sum of all components 
		template<typename T> typename T::type_t /*UF_API*/ product( const T& vector );						// 	Compute the product of all components 
		template<typename T> T /*UF_API*/ negate( const T& vector );										// 	Flip sign of all components
	// 	Writes to first value
		template<typename T> T& /*UF_API*/ multiply( T& left, const T& right );								// 	Multiplies two vectors of same type and size together
		template<typename T> T& /*UF_API*/ negate( T& vector );												// 	Flip sign of all components
		template<typename T> T& /*UF_API*/ normalize( T& vector ); 											// 	Normalizes a vector
	// 	Complex arithmetic
		template<typename T> typename T::type_t /*UF_API*/ dot( const T& left, const T& right ); 			// 	Compute the dot product between two vectors
		template<typename T> pod::Angle /*UF_API*/ angle( const T& a, const T& b ); 						// 	Compute the angle between two vectors
		
		template<typename T> T /*UF_API*/ lerp( const T& from, const T& to, typename T::type_t delta ); 	// 	Linearly interpolate between two vectors
		template<typename T> T /*UF_API*/ slerp( const T& from, const T& to, typename T::type_t delta ); 	// 	Spherically interpolate between two vectors
		
		template<typename T> typename T::type_t /*UF_API*/ distanceSquared( const T& a, const T& b ); 		// 	Gets the magnitude of the vector
		template<typename T> typename T::type_t /*UF_API*/ distance( const T& a, const T& b ); 				// 	Gets the magnitude of the vector
		template<typename T> typename T::type_t /*UF_API*/ magnitude( const T& vector ); 					// 	Gets the magnitude of the vector
		template<typename T> typename T::type_t /*UF_API*/ norm( const T& vector ); 						// 	Compute the norm of the vector
		template<typename T> T /*UF_API*/ normalize( const T& vector ); 									// 	Normalizes a vector
	// 	Quaternion ops
		template<typename T> pod::Matrix4t<typename T::type_t> matrix( const T& quaternion );
		template<typename T> pod::Quaternion<T> axisAngle( const pod::Vector3t<T>& axis, T angle );
		template<typename T> pod::Quaternion<T> unitVectors( const pod::Vector3t<T>& u, const pod::Vector3t<T>& v );
		template<typename T> pod::Quaternion<T> lookAt( const pod::Vector3t<T>& source, const pod::Vector3t<T>& destination );
		
		template<typename T> T conjugate( const T& quaternion );
		template<typename T> T inverse( const T& quaternion );
		template<typename T> T& conjugate( T& quaternion );
		template<typename T> T& inverse( T& quaternion );

		template<typename T> pod::Vector3t<T> eulerAngles( const pod::Quaternion<T>& quaternion );
		template<typename T> T pitch( const pod::Quaternion<T>& quaternion );
		template<typename T> T yaw( const pod::Quaternion<T>& quaternion );
		template<typename T> T roll( const pod::Quaternion<T>& quaternion );
		
		template<typename T> pod::Quaternion<T> fromMatrix( const pod::Matrix4t<T>& matrix );
	}
}

namespace uf {
	template<typename T = pod::Math::num_t>
	class /*UF_API*/ Quaternion {
public:
	// 	Easily access POD's type
		typedef pod::Quaternion<T> pod_t;
	// 	Replicate POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 4;
	protected:
	// 	POD storage
		Quaternion<T>::pod_t m_pod;
	public:
		T& x = m_pod.x;
		T& y = m_pod.y;
		T& z = m_pod.z;
		T& w = m_pod.w;
	public:
	// 	C-tor
		Quaternion(); 																	// initializes POD to 'def'
		Quaternion(T def); 																// initializes POD to 'def'
		Quaternion(T x, T y, T z, T w); 												// initializes POD to 'def'
		Quaternion(const Quaternion<T>::pod_t& pod); 									// copies POD altogether
		Quaternion(const T components[4]); 												// copies data into POD from 'components' (typed as C array)
		Quaternion(const uf::stl::vector<T>& components); 									// copies data into POD from 'components' (typed as uf::stl::vector<T>)
	// 	D-tor
		// Unneccesary
	// 	POD access
		Quaternion<T>::pod_t& data(); 													// 	Returns a reference of POD
		const Quaternion<T>::pod_t& data() const; 										// 	Returns a const-reference of POD
	// 	Alternative POD access
		T* get();																		// 	Returns a pointer to the entire array
		const T* get() const; 															// 	Returns a const-pointer to the entire array
		T& getComponent( std::size_t i );												// 	Returns a reference to a single element
		const T& getComponent( std::size_t i ) const; 									// 	Returns a const-reference to a single element
	// 	POD manipulation
		T* set(const T components[4]);													// 	Sets the entire array
		T& setComponent( std::size_t i, const T& value );								// 	Sets a single element
	// 	Validation
		bool isValid() const; 															// 	Checks if all components are valid (non NaN, inf, etc.)
	// 	Basic arithmetic
		inline uf::Quaternion<T>& multiply( const Quaternion<T>& quaternion );			// 	Multiplies two quaternions of same type and size together
		inline uf::Quaternion<T>  multiply( const Quaternion<T>& quaternion ) const;	// 	Multiplies two quaternions of same type and size together
		inline uf::Vector3t<T>    rotate( const Vector3t<T>& quaternion ) const;		// 	Multiplies a quaternion and a vector of same type and size together
		inline uf::Vector4t<T>    rotate( const Vector4t<T>& quaternion ) const;		// 	Multiplies a quaternion and a vector of same type and size together
		inline uf::Quaternion<T>& negate();												// 	Flip sign of all components
	// 	Complex arithmetic
		inline T dot( const Quaternion<T> right ) const; 								// 	Compute the dot product between two quaternions
		inline pod::Angle angle( const Quaternion<T>& b ) const; 						// 	Compute the angle between two quaternions
		
		inline uf::Quaternion<T> lerp( const Quaternion<T> to, typename T::type_t delta ) const; 	// 	Linearly interpolate between two quaternions
		inline uf::Quaternion<T> slerp( const Quaternion<T> to, typename T::type_t delta ) const; 	// 	Spherically interpolate between two quaternions
		
		inline T distanceSquared( const Quaternion<T> b ) const; 						// 	Compute the distance between two quaternions (doesn't sqrt)
		inline T distance( const Quaternion<T> b ) const; 								// 	Compute the distance between two quaternions
		inline T magnitude() const; 													// 	Gets the magnitude of the quaternion
		inline T norm() const; 															// 	Compute the norm of the quaternion
		
		inline uf::Quaternion<T>& normalize(); 											// 	Normalizes a quaternion
		uf::Quaternion<T> getNormalized() const; 										// 	Return a normalized quaternion
	// 	Quaternion ops
		inline uf::Matrix4t<T> matrix() const;
		inline uf::Quaternion<T>& axisAngle( const Vector3t<T>& axis, T angle );
		inline uf::Quaternion<T>& unitVectors( const Vector3t<T>& u, const Vector3t<T>& v );
		
		inline uf::Quaternion<T>& conjugate();
		inline uf::Quaternion<T>& inverse();
		
		inline uf::Quaternion<T> getConjugate() const;
		inline uf::Quaternion<T> getInverse() const;
		inline uf::stl::string toString() const;
	// 	Overloaded ops
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Quaternion<T> operator-() const; 										// 	Negation
		inline Quaternion<T> operator*( const Quaternion<T>& quaternion ) const; 		// 	Multiplication between two quaternions
		inline Vector3t<T> operator*( const Vector3t<T>& vector ) const; 				// 	Multiplication between a quaternion and a vector (rotates vector)
		inline Vector4t<T> operator*( const Vector4t<T>& vector ) const; 				// 	Multiplication between a quaternion and a vector (rotates vector)
		inline Matrix4t<T> operator*( const Matrix4t<T>& matrix ) const; 				// 	Multiplication between a quaternion and a matrix
		inline Quaternion<T>& operator *=( const Quaternion<T>& quaternion ); 			// 	Multiplication set between two quaternions
		inline bool operator==( const Quaternion<T>& quaternion ) const; 				// 	Equality check between two quaternions (equals)
		inline bool operator!=( const Quaternion<T>& quaternion ) const; 				// 	Equality check between two quaternions (not equals)
		inline bool operator<( const Quaternion<T>& quaternion ) const; 				// 	Equality check between two quaternions (less than)
		inline bool operator<=( const Quaternion<T>& quaternion ) const; 				// 	Equality check between two quaternions (less than or equals)
		inline bool operator>( const Quaternion<T>& quaternion ) const; 				// 	Equality check between two quaternions (greater than)
		inline bool operator>=( const Quaternion<T>& quaternion ) const; 				// 	Equality check between two quaternions (greater than or equals)
		
		inline operator pod_t&() { return this->m_pod; }
		inline operator const pod_t&() const { return this->m_pod; }
	};
}


#include "quaternion/quaternion.inl"
#ifdef UF_USE_GLM_TMP
	#undef UF_USE_GLM
	#undef UF_USE_GLM_TMP
#endif