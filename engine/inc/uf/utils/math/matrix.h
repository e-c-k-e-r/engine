#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>
#ifdef UF_USE_GLM
	#include "glm.h"
#endif

#include "math.h"
namespace pod {
	template<typename T = pod::Math::num_t, size_t R = 4, size_t C = R> 
#if UF_MATRIX_ALIGNED
	struct /*UF_API*/ alignas(16) Matrix {
#else
	struct /*UF_API*/ /*alignas(16)*/ Matrix {
#endif
	// 	n-dimensional/unspecialized matrix access
		T components[R*C] = {};
	//	T components[R][C] = {};
	//	T* array = (T*) this;
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		static const uint_fast8_t rows = R;
		static const uint_fast8_t columns = C;
	// 	Overload access
		// Accessing via subscripts
		inline T& operator[](uint_fast8_t i);
		inline const T& operator[](uint_fast8_t i) const;
		
		inline T& operator()(uint_fast8_t r, uint_fast8_t c);
		inline const T& operator()(uint_fast8_t r, uint_fast8_t c) const;

		// Arithmetic
		Matrix<T,R,C> operator()() const; 								// 	Creation
		Matrix<T,R,C> operator-() const; 								// 	Negation
		Matrix<T,R,C> operator*( const Matrix<T,R,C>& matrix ) const; 	// 	Multiplication between two matrices
		Matrix<T,R,C> operator*( T scalar ) const; 						// 	Multiplication between two matrices
		Matrix<T,R,C> operator+( const Matrix<T,R,C>& matrix ) const; 	// 	Multiplication between two matrices
		Matrix<T,R,C>& operator *=( const Matrix<T,R,C>& matrix ); 		// 	Multiplication set between two matrices
		bool operator==( const Matrix<T,R,C>& matrix ) const; 			// 	Equality check between two matrices (equals)
		bool operator!=( const Matrix<T,R,C>& matrix ) const; 			// 	Equality check between two matrices (not equals)
	};
	template<typename T = pod::Math::num_t> using Matrix3t = Matrix<T,3>;
	typedef Matrix3t<> Matrix3;
	typedef Matrix3t<float> Matrix3f;

	template<typename T = pod::Math::num_t> using Matrix4t = Matrix<T,4>;
	typedef Matrix4t<> Matrix4;
	typedef Matrix4t<float> Matrix4f;
}

namespace uf {
	namespace matrix {
		extern bool UF_API reverseInfiniteProjection;

		template<typename T=pod::Math::num_t> pod::Matrix4t<T> /*UF_API*/ identity();

		template<typename T=pod::Math::num_t> pod::Matrix4t<T> /*UF_API*/ initialize( const T* );
		template<typename T=pod::Math::num_t> pod::Matrix4t<T> /*UF_API*/ initialize( const uf::stl::vector<T>& );
		template<typename T> pod::Matrix<typename T::type_t, T::columns, T::columns> /*UF_API*/ identityi();

	// 	Equality checking
		template<typename T=pod::Matrix4> int /*UF_API*/ compareTo( const T& left, const T& right ); 			// 	Equality check between two matrices (less than)
		template<typename T=pod::Matrix4> bool /*UF_API*/ equals( const T& left, const T& right ); 						// 	Equality check between two matrices (equals)
	// 	Basic arithmetic
	//	template<typename T=pod::Matrix4> pod::Matrix<typename T::type_t, C, C> /*UF_API*/ multiply( const T& left, const T& right );						// 	Multiplies two matrices of same type and size together
		template<typename T, typename U> pod::Matrix<typename T::type_t, T::columns, T::columns> multiply( const T& left, const U& right );						// 	Multiplies two matrices of same type and size together
		template<typename T> pod::Matrix<T,4,4> multiply( const pod::Matrix<T,4,4>& left, const pod::Matrix<T,4,4>& right );						// 	Multiplies two matrices of same type and size together
		template<typename T=pod::Matrix4> T /*UF_API*/ transpose( const T& matrix );										// 	Flip sign of all components
		template<typename T=pod::Matrix4> T /*UF_API*/ inverse( const T& matrix );										// 	Flip sign of all components
		template<typename T=pod::Math::num_t> pod::Vector3t<T> multiply( const pod::Matrix4t<T>& mat, const pod::Vector3t<T>& vector, T w = 1, bool = false );
		template<typename T=pod::Math::num_t> pod::Vector4t<T> multiply( const pod::Matrix4t<T>& mat, const pod::Vector4t<T>& vector, bool = false );
		
		template<typename T=pod::Matrix4> T /*UF_API*/ multiplyAll( const T& matrix, typename T::type_t scalar );
		template<typename T=pod::Matrix4> T /*UF_API*/ add( const T& lhs, const T& rhs );
	// 	Writes to first value
	//	template<typename T=pod::Matrix4> pod::Matrix<typename T::type_t, C, C>& /*UF_API*/ multiply( T& left, const T& right );								// 	Multiplies two matrices of same type and size together
		template<typename T, typename U> pod::Matrix<typename T::type_t, T::columns, T::columns> multiply( T& left, const U& right );								// 	Multiplies two matrices of same type and size together
		template<typename T> pod::Matrix<typename T::type_t, T::columns, T::columns> multiply( T& left, const T& right );
		template<typename T=pod::Matrix4> T& /*UF_API*/ invert( T& matrix );												// 	Flip sign of all components
	// 	Complex arithmetic
		template<typename T=pod::Matrix4> T /*UF_API*/ translate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> T /*UF_API*/ rotate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> T /*UF_API*/ scale( const T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> T& /*UF_API*/ translate( T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> T& /*UF_API*/ rotate( T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> T& /*UF_API*/ scale( T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> pod::Vector3t<typename T::type_t> /*UF_API*/ eulerAngles( const T& matrix );

		template<typename T=pod::Math::num_t> pod::Matrix4t<T> /*UF_API*/ orthographic( T, T, T, T, T, T );
		template<typename T=pod::Math::num_t> pod::Matrix4t<T> /*UF_API*/ orthographic( T, T, T, T );
		template<typename T=pod::Math::num_t> pod::Matrix4t<T> inline /*UF_API*/ orthographic( const pod::Vector2t<T>& lr, const pod::Vector2t<T>& bt, const pod::Vector2t<T>& nf ) {
			return orthographic<T>( lr.x, lr.y, bt.x, bt.y, nf.x, nf.y );
		}
		template<typename T=pod::Math::num_t> pod::Matrix4t<T> inline /*UF_API*/ orthographic( const pod::Vector2t<T>& lr, const pod::Vector2t<T>& bt ) {
			return orthographic<T>( lr.x, lr.y, bt.x, bt.y );
		}

		template<typename T=pod::Math::num_t> pod::Matrix4t<T> /*UF_API*/ perspective( T, T, T, T );
		template<typename T=pod::Math::num_t> pod::Matrix4t<T> inline /*UF_API*/ perspective( T fov, T raidou, const pod::Vector2f& range ) {
			return perspective( fov, raidou, range.x, range.y );
		}
		template<typename T=pod::Math::num_t> pod::Matrix4t<T> inline /*UF_API*/ perspective( T fov, const pod::Vector2ui& size, const pod::Vector2f& range ) {
			return perspective( fov, (T) size.x / (T) size.y, range.x, range.y );
		}
	// 	Setting
		template<typename T=pod::Matrix4> T& /*UF_API*/ copy( T& destination, const T& source );
		template<typename T=pod::Matrix4> T& /*UF_API*/ copy( T& destination, typename T::type_t* const source );

		template<typename T, size_t R, size_t C = R> uf::stl::string /*UF_API*/ toString( const pod::Matrix<T,R,C>& v );
		template<typename T, size_t R, size_t C> ext::json::Value /*UF_API*/ encode( const pod::Matrix<T,R,C>& v, const ext::json::EncodingSettings& = {} );
		template<typename T, size_t R, size_t C> pod::Matrix<T,R,C>& /*UF_API*/ decode( const ext::json::Value& v, pod::Matrix<T,R,C>& );
		template<typename T, size_t R, size_t C> pod::Matrix<T,R,C> /*UF_API*/ decode( const ext::json::Value& v, const pod::Matrix<T,R,C>& = uf::matrix::identity() );
	}
}

namespace uf {
	template<typename T = pod::Math::num_t, std::size_t R = 4, std::size_t C = R>
	class /*UF_API*/ Matrix {
	public:
	// 	Easily access POD's type
		typedef pod::Matrix<T,R,C> pod_t;
	// 	Replicate POD information
		typedef T type_t;
		typedef T* container_t;
		static const uint_fast8_t rows = R;
		static const uint_fast8_t columns = C;
	protected:
	// 	POD storage
		Matrix<T,R,C>::pod_t m_pod;
	public:
	// 	C-tor
		Matrix(); 																		// initializes POD to 'def'
		Matrix(const Matrix<T,R,C>::pod_t& pod); 										// copies POD altogether
		Matrix(const T components[R][C]); 												// copies data into POD from 'components' (typed as C array)
		Matrix(const T components[R*C]); 												// copies data into POD from 'components' (typed as C array)
		Matrix(const uf::stl::vector<T>& components); 										// copies data into POD from 'components' (typed as std::matrix<T>)
	// 	D-tor
		// Unneccesary
	// 	POD access
		Matrix<T,R,C>::pod_t& data(); 													// 	Returns a reference of POD
		const Matrix<T,R,C>::pod_t& data() const; 										// 	Returns a const-reference of POD
		
		template<typename Q> typename Matrix<Q,R,C>::pod_t convert() const; 						// 	Returns a POD converted
	// 	Alternative POD access
		T* get();																		// 	Returns a pointer to the entire array
		const T* get() const; 															// 	Returns a const-pointer to the entire array
		T& getComponent( uint_fast8_t i );												// 	Returns a reference to a single element
		const T& getComponent( uint_fast8_t i ) const; 									// 	Returns a const-reference to a single element
	// 	POD manipulation
		T* set(const T components[R][C]);												// 	Sets the entire array
		T* set(const T components[R*C]);												// 	Sets the entire array
		T& setComponent( uint_fast8_t i, const T& value );								// 	Sets a single element
	// 	Validation
		bool isValid() const; 															// 	Checks if all components are valid (non NaN, inf, etc.)
	// 	Basic arithmetic
		inline uf::Matrix<T,C,C>  multiply( const Matrix<T,R,C>& matrix ) const;		// 	Multiplies two matrices of same type and size together
		inline uf::Matrix<T,C,C>  multiply( const Matrix<T,R,C>& matrix );				// 	Multiplies two matrices of same type and size together
		inline pod::Vector3t<T> multiply( const pod::Vector3t<T>& vector ) const;
		inline pod::Vector4t<T> multiply( const pod::Vector4t<T>& vector ) const;
		inline uf::Matrix<T,R,C>& negate();												// 	Flip sign of all components
		inline uf::Matrix<T,R,C>& translate( const pod::Vector3t<T>& vector );
		inline uf::Matrix<T,R,C>& rotate( const pod::Vector3t<T>& vector );
		inline uf::Matrix<T,R,C>& scale( const pod::Vector3t<T>& vector );
		inline uf::Matrix<T,R,C>& invert();
		inline uf::Matrix<T,R,C>  inverse() const;
		template<typename U>
		inline uf::Matrix<T,C,C> multiply( const U& matrix ) const;					// 	Multiplies two matrices of same type and size together
		template<typename U>
		inline uf::Matrix<T,C,C> multiply( const U& matrix );							// 	Multiplies two matrices of same type and size together
	// 	Overloaded ops
		// Accessing via subscripts
		T& operator[](uint_fast8_t i);
		const T& operator[](uint_fast8_t i) const;
		// Arithmetic
		inline Matrix<T,R,C> operator-() const; 										// 	Negation
		inline Matrix<T,C,C> operator*( const Matrix<T,R,C>& matrix ) const; 			// 	Multiplication between two matrices
		inline Matrix<T,C,C>& operator *=( const Matrix<T,R,C>& matrix ); 				// 	Multiplication set between two matrices
		template<typename U> inline Matrix<T,C,C> operator*( const U& matrix ) const; 	// 	Multiplication between two matrices
		inline bool operator==( const Matrix<T,R,C>& matrix ) const; 					// 	Equality check between two matrices (equals)
		inline bool operator!=( const Matrix<T,R,C>& matrix ) const; 					// 	Equality check between two matrices (not equals)
		
		inline operator pod_t&() { return this->m_pod; }
		inline operator const pod_t&() const { return this->m_pod; } 
	};
	template<typename T = pod::Math::num_t> using Matrix4t = Matrix<T,4>;
	typedef Matrix4t<> Matrix4;
	typedef Matrix4t<float> Matrix4f;
}

#include <sstream>
namespace uf {
	namespace string {
		template<typename T, size_t R, size_t C>
		uf::stl::string /*UF_API*/ toString( const pod::Matrix<T,R,C>& v );
	}
}
namespace ext {
	namespace json {
		template<typename T, size_t R, size_t C = R> ext::json::Value /*UF_API*/ encode( const pod::Matrix<T,R,C>& v );
		template<typename T, size_t R, size_t C = R> pod::Matrix<T,R,C>& /*UF_API*/ decode( const ext::json::Value& v, pod::Matrix<T,R,C>& );
		template<typename T, size_t R, size_t C = R> pod::Matrix<T,R,C> /*UF_API*/ decode( const ext::json::Value& v, const pod::Matrix<T,R,C>& = uf::matrix::identity() );
	}
}

#include "matrix/matrix.inl"