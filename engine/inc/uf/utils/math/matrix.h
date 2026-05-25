#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>

#include "math.h"
namespace pod {
	template<typename T = NUM, size_t R = 4, size_t C = R> 
	struct /*UF_API*/ Matrix {
	// 	n-dimensional/unspecialized matrix access
		union {
			T components[R*C] = {};
			pod::Vector<T, C> _rows[R]; // to-do: better name
			pod::Vector<T, R> _columns[C]; // to-do: better name
		};
	// 	POD information
		typedef T type_t;
		typedef T* container_t;
		typedef pod::Vector<T, R> row_t;
		typedef pod::Vector<T, C> col_t;
		static const size_t rows = R;
		static const size_t columns = C;
	// 	Overload access
		// Accessing via subscripts
		FORCE_INLINE T& operator[](size_t i);
		FORCE_INLINE const T& operator[](size_t i) const;
		
		FORCE_INLINE T& operator()(size_t r, size_t c);
		FORCE_INLINE const T& operator()(size_t r, size_t c) const;

		// Arithmetic
		FORCE_INLINE Matrix<T,R,C> operator*( const Matrix<T,R,C>& matrix ) const;
		FORCE_INLINE Matrix<T,R,C> operator*( T scalar ) const;
		FORCE_INLINE Matrix<T,R,C> operator+( const Matrix<T,R,C>& matrix ) const;
		FORCE_INLINE Matrix<T,R,C> operator-( const Matrix<T,R,C>& matrix ) const;
		FORCE_INLINE Matrix<T,R,C>& operator *=( const Matrix<T,R,C>& matrix );
		FORCE_INLINE Matrix<T,R,C>& operator +=( const Matrix<T,R,C>& matrix );
		FORCE_INLINE Matrix<T,R,C>& operator -=( const Matrix<T,R,C>& matrix );
		FORCE_INLINE bool operator==( const Matrix<T,R,C>& matrix ) const;
		FORCE_INLINE bool operator!=( const Matrix<T,R,C>& matrix ) const;
	};

	template<typename T = NUM> using Matrix2t = Matrix<T,2>;
	typedef Matrix2t<> Matrix2;
	typedef Matrix2t<float> Matrix2f;

	template<typename T = NUM> using Matrix3t = Matrix<T,3>;
	typedef Matrix3t<> Matrix3;
	typedef Matrix3t<float> Matrix3f;

	template<typename T = NUM> using Matrix4t = Matrix<T,4>;
	typedef Matrix4t<> Matrix4;
	typedef Matrix4t<float> Matrix4f;
}

namespace uf {
	namespace matrix {
		extern bool UF_API reverseInfiniteProjection;

		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ identity();

		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ initialize( const T* );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ initialize( const uf::stl::vector<T>& );
		template<typename T> /*FORCE_INLINE*/ pod::Matrix<typename T::type_t, T::columns, T::columns> /*UF_API*/ identityi();

		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ bool /*UF_API*/ equals( const T& left, const T& right, float eps = 1.0e-6f );

		template<typename T, typename U> /*FORCE_INLINE*/ pod::Matrix<typename T::type_t, T::columns, T::columns> multiply( const T& left, const U& right );
		template<typename T> /*FORCE_INLINE*/ pod::Matrix4t<T> multiply( const pod::Matrix4t<T>& left, const pod::Matrix4t<T>& right );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T /*UF_API*/ transpose( const T& matrix );
		
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix2t<T> inverse(const pod::Matrix2t<T>& mat );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix3t<T> inverse(const pod::Matrix3t<T>& mat );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> inverse(const pod::Matrix4t<T>& mat );

		template<typename T=NUM> /*FORCE_INLINE*/ pod::Vector2t<T> multiply(const pod::Matrix2t<T>& mat, const pod::Vector2t<T>& v );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Vector3t<T> multiply(const pod::Matrix3t<T>& mat, const pod::Vector3t<T>& v );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Vector3t<T> multiply( const pod::Matrix4t<T>& mat, const pod::Vector3t<T>& vector, T w = 1, bool = false );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Vector4t<T> multiply( const pod::Matrix4t<T>& mat, const pod::Vector4t<T>& vector, bool = false );
		template<typename T=NUM, size_t M, size_t N> /*FORCE_INLINE*/ pod::Matrix<T,M,N> /*UF_API*/ outerProduct( const pod::Vector<T,M>& a, const pod::Vector<T,N>& b );
		
		template<typename T=NUM, size_t R, size_t C> /*FORCE_INLINE*/ pod::Vector<T, R> diagonal(const pod::Matrix<T, R, C>& mat );
		template<typename T=NUM, size_t N> /*FORCE_INLINE*/ pod::Matrix<T, N, N> diagonal(const pod::Vector<T, N>& vec );

		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T /*UF_API*/ multiplyAll( const T& matrix, typename T::type_t scalar );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T /*UF_API*/ add( const T& lhs, const T& rhs );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T /*UF_API*/ subtract( const T& lhs, const T& rhs );

		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T& /*UF_API*/ inverse_( T& matrix );
		template<typename T, typename U> /*FORCE_INLINE*/ pod::Matrix<typename T::type_t, T::columns, T::columns> multiply_( T& left, const U& right );
		template<typename T> /*FORCE_INLINE*/ pod::Matrix<typename T::type_t, T::columns, T::columns> multiply_( T& left, const T& right );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T& /*UF_API*/ translate_( T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T& /*UF_API*/ rotate_( T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T& /*UF_API*/ scale_( T& matrix, const pod::Vector3t<typename T::type_t>& vector );

		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T /*UF_API*/ translate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T /*UF_API*/ rotate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T /*UF_API*/ scale( const T& matrix, const pod::Vector3t<typename T::type_t>& vector );

		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ pod::Vector3t<typename T::type_t> extractTranslation( const T& matrix );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ pod::Vector3t<typename T::type_t> extractScale( const T& matrix );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ pod::Vector4t<typename T::type_t> extractRotation( const T& matrix );

		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ pod::Vector3t<typename T::type_t> /*UF_API*/ eulerAngles( const T& matrix );

		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ orthographic( T, T, T, T, T, T );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ orthographic( T, T, T, T );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ orthographic( const pod::Vector2t<T>& lr, const pod::Vector2t<T>& bt, const pod::Vector2t<T>& nf ) {
			return orthographic<T>( lr.x, lr.y, bt.x, bt.y, nf.x, nf.y );
		}
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ orthographic( const pod::Vector2t<T>& lr, const pod::Vector2t<T>& bt ) {
			return orthographic<T>( lr.x, lr.y, bt.x, bt.y );
		}

		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ perspective( T, T, T, T );
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ perspective( T fov, T raidou, const pod::Vector2f& range ) {
			return perspective( fov, raidou, range.x, range.y );
		}
		template<typename T=NUM> /*FORCE_INLINE*/ pod::Matrix4t<T> /*UF_API*/ perspective( T fov, const pod::Vector2ui& size, const pod::Vector2f& range ) {
			return perspective( fov, (T) size.x / (T) size.y, range.x, range.y );
		}
	// 	Setting
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T& /*UF_API*/ copy( T& destination, const T& source );
		template<typename T=pod::Matrix4> /*FORCE_INLINE*/ T& /*UF_API*/ copy( T& destination, typename T::type_t* const source );

		template<typename T, size_t R, size_t C = R> /*FORCE_INLINE*/ uf::stl::string /*UF_API*/ toString( const pod::Matrix<T,R,C>& v );
		template<typename T, size_t R, size_t C> /*FORCE_INLINE*/ ext::json::Value /*UF_API*/ encode( const pod::Matrix<T,R,C>& v, const ext::json::EncodingSettings& = {} );
		template<typename T, size_t R, size_t C> /*FORCE_INLINE*/ pod::Matrix<T,R,C>& /*UF_API*/ decode( const ext::json::Value& v, pod::Matrix<T,R,C>& );
		template<typename T, size_t R, size_t C> /*FORCE_INLINE*/ pod::Matrix<T,R,C> /*UF_API*/ decode( const ext::json::Value& v, const pod::Matrix<T,R,C>& = uf::matrix::identity() );
	}
}

#include <sstream>
namespace uf {
	namespace string {
		template<typename T, size_t R, size_t C>
		FORCE_INLINE uf::stl::string /*UF_API*/ toString( const pod::Matrix<T,R,C>& v );
	}
}
namespace ext {
	namespace json {
		template<typename T, size_t R, size_t C = R> FORCE_INLINE ext::json::Value /*UF_API*/ encode( const pod::Matrix<T,R,C>& v );
		template<typename T, size_t R, size_t C = R> FORCE_INLINE pod::Matrix<T,R,C>& /*UF_API*/ decode( const ext::json::Value& v, pod::Matrix<T,R,C>& );
		template<typename T, size_t R, size_t C = R> FORCE_INLINE pod::Matrix<T,R,C> /*UF_API*/ decode( const ext::json::Value& v, const pod::Matrix<T,R,C>& = uf::matrix::identity() );
	}
}

#include "matrix/matrix.inl"