#define FOR_EACH_2D( R, C, F ) for_each_index<R>([&](auto r) { for_each_index<C>([&](auto c) F ); });
#define ROW_MAJOR_INDEX( R, C, r, c ) (r * C + c)
#define COL_MAJOR_INDEX( R, C, r, c ) (c * R + r)

#define INDEX( R, C, r, c ) COL_MAJOR_INDEX( R, C, r, c )

template<typename T, size_t R, size_t C>
FORCE_INLINE T& pod::Matrix<T,R,C>::operator[](size_t i) {
	return this->components[i];
}
template<typename T, size_t R, size_t C>
FORCE_INLINE const T& pod::Matrix<T,R,C>::operator[](size_t i) const {
	return this->components[i];
}
template<typename T, size_t R, size_t C>
FORCE_INLINE T& pod::Matrix<T,R,C>::operator()(size_t r, size_t c) {
	return this->components[INDEX( R, C, r, c )];
}
template<typename T, size_t R, size_t C>
FORCE_INLINE const T& pod::Matrix<T,R,C>::operator()(size_t r, size_t c) const {
	return this->components[INDEX( R, C, r, c )];
}

template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::identity() {
	pod::Matrix4t<T> matrix;
	FOR_EACH_2D(4, 4, {
		matrix(r, c) = (r == c ? T{1} : T{0});
	});
	return matrix;
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::initialize( const T* list ) {
	pod::Matrix4t<T> matrix;
	FOR_EACH(16, {
		matrix.components[i] = list[i];
	});

	return matrix;
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::initialize( const uf::stl::vector<T>& list ) {
	pod::Matrix4t<T> matrix;
	if ( list.size() != 16 ) return matrix;
	FOR_EACH(16, {
		matrix.components[i] = list[i];
	});

	return matrix;
}
template<typename T> pod::Matrix<typename T::type_t, T::columns, T::columns> uf::matrix::identityi(){
	pod::Matrix<typename T::type_t, T::columns, T::columns> matrix;
	FOR_EACH_2D(T::rows, T::columns, {
		matrix(r, c) = (r == c ? 1 : 0);
	});

	return matrix;
}
template<typename T, size_t R, size_t C>
FORCE_INLINE pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator*( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::multiply(*this, matrix);
}
template<typename T, size_t R, size_t C>
FORCE_INLINE pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator*( T scalar ) const {
	return uf::matrix::multiplyAll(*this, scalar);
}
template<typename T, size_t R, size_t C>
FORCE_INLINE pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator+( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::add(*this, matrix);
}
template<typename T, size_t R, size_t C>
FORCE_INLINE pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator-( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::subtract(*this, matrix);
}
template<typename T, size_t R, size_t C>
FORCE_INLINE pod::Matrix<T,R,C>& pod::Matrix<T,R,C>::operator*=( const Matrix<T,R,C>& matrix ) {
	return uf::matrix::multiply_(*this, matrix);
}
template<typename T, size_t R, size_t C>
FORCE_INLINE pod::Matrix<T,R,C>& pod::Matrix<T,R,C>::operator+=( const Matrix<T,R,C>& matrix ) {
	return *this = uf::matrix::add(*this, matrix); // to-do: non-const 
}
template<typename T, size_t R, size_t C>
FORCE_INLINE pod::Matrix<T,R,C>& pod::Matrix<T,R,C>::operator-=( const Matrix<T,R,C>& matrix ) {
	return *this = uf::matrix::subtract(*this, matrix); // to-do: non-const 
}
template<typename T, size_t R, size_t C>
FORCE_INLINE bool pod::Matrix<T,R,C>::operator==( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::equals( *this, matrix );
}
template<typename T, size_t R, size_t C>
FORCE_INLINE bool pod::Matrix<T,R,C>::operator!=( const Matrix<T,R,C>& matrix ) const {
	return !uf::matrix::equals( *this, matrix );
}
template<typename T> bool uf::matrix::equals( const T& left, const T& right, float eps ) {
#if UF_USE_SIMD
	if constexpr (std::is_same_v<T,float>) {
		return uf::simd::matEquals( left, right, eps );
	}
#endif
	bool result = true;
	FOR_EACH(T::rows * T::columns, {
		if ( fabs(left[i] - right[i]) > eps ) result = false;
	});
	return result;
}
template<typename T> pod::Matrix<T,4,4> uf::matrix::multiply( const pod::Matrix<T,4,4>& left, const pod::Matrix<T,4,4>& right ) {
	pod::Matrix<T,4,4> res;

#if UF_USE_SIMD
	if constexpr (std::is_same_v<T,float>) {
		return uf::simd::matMult( left, right );
	}
#endif
#if UF_ENV_DREAMCAST
// 	kallistios has dedicated SH4 asm for these or something
	mat_load( (matrix_t*) &left[0] );
	mat_apply( (matrix_t*) &right[0] );
	mat_store( (matrix_t*) &res[0]);

// 	gives very wrong output, not sure why
//	MATH_Load_Matrix_Product( (ALL_FLOATS_STRUCT*) &left[0], (ALL_FLOATS_STRUCT*) &right[0] );
//	MATH_Store_XMTRX( (ALL_FLOATS_STRUCT*) &res[0]);
	return res;
#else
	FOR_EACH_2D(4, 4, {
		T sum = T{0};
		for (size_t k = 0; k < 4; ++k) {
			sum += left(r, k) * right(k, c);
		}
		res(r, c) = sum;
	});
	return res;
#endif
}
template<typename T, typename U> pod::Matrix<typename T::type_t, T::columns, T::columns> uf::matrix::multiply( const T& left, const U& right ) {
	pod::Matrix<typename T::type_t,T::rows,T::columns> res;

	float* dstPtr = &res[0];
	const float* leftPtr = &left[0];

	#pragma unroll // GCC unroll T::rows
	for ( auto i = 0; i < T::rows; ++i) {
		#pragma unroll // GCC unroll T::columns
		for ( auto j = 0; j < T::columns; ++j) {
			const float* rightPtr = &right[0] + j;

			float sum = leftPtr[0] * rightPtr[0];
			#pragma unroll // GCC unroll T::columns - 1
			for ( auto n = 1; n < T::columns; ++n) {
				rightPtr += T::columns;
				sum += leftPtr[n] * rightPtr[0];
			}
			*dstPtr++ = sum;
		}
		leftPtr += T::columns;
	}

	return res;
}

template<typename T, size_t M, size_t N> pod::Matrix<T,M,N> /*UF_API*/ uf::matrix::outerProduct( const pod::Vector<T,M>& a, const pod::Vector<T,N>& b ) {
	pod::Matrix<T, M, N> m{};
	for ( auto i = 0; i < M; ++i ) {
		for ( auto j = 0; j < N; ++j ) {
			m(i, j) = a[i] * b[j];
		}
	}
	return m;
}

template<typename T, size_t R, size_t C> pod::Vector<T, R> /*UF_API*/ uf::matrix::diagonal(const pod::Matrix<T, R, C>& mat ) {
	pod::Vector<T, R> vector;
	FOR_EACH(R, {
		vector[i] = m(i, i);
	});
	return vector;
}
template<typename T, size_t N> pod::Matrix<T, N, N> /*UF_API*/ uf::matrix::diagonal(const pod::Vector<T, N>& vector ) {
	pod::Matrix<T, N, N> matrix;
	FOR_EACH(N, {
		matrix(i, i) = vector[i];
	});
	return matrix;
}

template<typename T> T /*UF_API*/ uf::matrix::multiplyAll( const T& m, typename T::type_t scalar ) {
	T matrix;

	FOR_EACH(T::rows * T::columns, {
		matrix[i] = m[i] * scalar;
	});

	return matrix;
}
template<typename T> T /*UF_API*/ uf::matrix::add( const T& lhs, const T& rhs ) {
	T matrix;

	FOR_EACH(T::rows * T::columns, {
		matrix[i] = lhs[i] + rhs[i];
	});

	return matrix;
}
template<typename T> T /*UF_API*/ uf::matrix::subtract( const T& lhs, const T& rhs ) {
	T matrix;

	FOR_EACH(T::rows * T::columns, {
		matrix[i] = lhs[i] - rhs[i];
	});

	return matrix;
}
template<typename T> T uf::matrix::transpose( const T& matrix ) {
#if UF_USE_SIMD
	if constexpr (std::is_same_v<T,float> && T::rows == 4 && T::columns == 4 ) {
		return uf::simd::matTranspose( matrix );
	}
#endif
	T transpose;
	FOR_EACH_2D(T::rows, T::columns, {
		transpose(c, r) = matrix(r, c);
	});

	return transpose;
}
template<typename T> pod::Matrix2t<T> uf::matrix::inverse( const pod::Matrix2t<T>& m ) {
	T det = m[0] * m[3] - m[1] * m[2];
	if ( std::fabs(det) < 1e-12f ) return m;

	T invDet = 1 / det;
	
	return pod::Matrix2t<T>{
		 m[3] * invDet, -m[1] * invDet,
		-m[2] * invDet, m[0] * invDet,
	};
}

template<typename T> pod::Matrix3t<T> uf::matrix::inverse( const pod::Matrix3t<T>& m ) {
	const T* a = &m[0];
	T det = a[0]*(a[4]*a[8] - a[5]*a[7]) - a[1]*(a[3]*a[8] - a[5]*a[6]) + a[2]*(a[3]*a[7] - a[4]*a[6]);
	if ( std::fabs(det) < 1e-12f ) return m; // singular
	T invDet = static_cast<T>(1) / det;
	return pod::Matrix3t<T>{
		(a[4]*a[8] - a[5]*a[7]) * invDet, (a[2]*a[7] - a[1]*a[8]) * invDet, (a[1]*a[5] - a[2]*a[4]) * invDet,
		(a[5]*a[6] - a[3]*a[8]) * invDet, (a[0]*a[8] - a[2]*a[6]) * invDet, (a[2]*a[3] - a[0]*a[5]) * invDet,
		(a[3]*a[7] - a[4]*a[6]) * invDet, (a[1]*a[6] - a[0]*a[7]) * invDet, (a[0]*a[4] - a[1]*a[3]) * invDet,
	};
}

template<typename T> pod::Matrix4t<T> uf::matrix::inverse( const pod::Matrix4t<T>& m ) {
	const T* a = &m[0];
	pod::Matrix4t<T> inv;

	inv[0]  =   a[5] * (a[10]*a[15] - a[11]*a[14]) - a[9] * (a[6]*a[15]  - a[7]*a[14]) + a[13]* (a[6]*a[11]  - a[7]*a[10]);
	inv[4]  = - a[4] * (a[10]*a[15] - a[11]*a[14]) + a[8] * (a[6]*a[15]  - a[7]*a[14]) - a[12]* (a[6]*a[11]  - a[7]*a[10]);
	inv[8]  =   a[4] * (a[9]*a[15]  - a[11]*a[13]) - a[8] * (a[5]*a[15]  - a[7]*a[13]) + a[12]* (a[5]*a[11]  - a[7]*a[9]);
	inv[12] = - a[4] * (a[9]*a[14]  - a[10]*a[13]) + a[8] * (a[5]*a[14]  - a[6]*a[13]) - a[12]* (a[5]*a[10]  - a[6]*a[9]);
	inv[1]  = - a[1] * (a[10]*a[15] - a[11]*a[14]) + a[9] * (a[2]*a[15]  - a[3]*a[14]) - a[13]* (a[2]*a[11]  - a[3]*a[10]);
	inv[5]  =   a[0] * (a[10]*a[15] - a[11]*a[14]) - a[8] * (a[2]*a[15]  - a[3]*a[14]) + a[12]* (a[2]*a[11]  - a[3]*a[10]);
	inv[9]  = - a[0] * (a[9]*a[15]  - a[11]*a[13]) + a[8] * (a[1]*a[15]  - a[3]*a[13]) - a[12]* (a[1]*a[11]  - a[3]*a[9]);
	inv[13] =   a[0] * (a[9]*a[14]  - a[10]*a[13]) - a[8] * (a[1]*a[14]  - a[2]*a[13]) + a[12]* (a[1]*a[10]  - a[2]*a[9]);
	inv[2]  =   a[1] * (a[6]*a[15]  - a[7]*a[14]) - a[5] * (a[2]*a[15]  - a[3]*a[14]) + a[13]* (a[2]*a[7]   - a[3]*a[6]);
	inv[6]  = - a[0] * (a[6]*a[15]  - a[7]*a[14]) + a[4] * (a[2]*a[15]  - a[3]*a[14]) - a[12]* (a[2]*a[7]   - a[3]*a[6]);
	inv[10] =   a[0] * (a[5]*a[15]  - a[7]*a[13]) - a[4] * (a[1]*a[15]  - a[3]*a[13]) + a[12]* (a[1]*a[7]   - a[3]*a[5]);
	inv[14] = - a[0] * (a[5]*a[14]  - a[6]*a[13]) + a[4] * (a[1]*a[14]  - a[2]*a[13]) - a[12]* (a[1]*a[6]   - a[2]*a[5]);
	inv[3]  = - a[1] * (a[6]*a[11]  - a[7]*a[10]) + a[5] * (a[2]*a[11]  - a[3]*a[10]) - a[9] * (a[2]*a[7]   - a[3]*a[6]);
	inv[7]  =   a[0] * (a[6]*a[11]  - a[7]*a[10]) - a[4] * (a[2]*a[11]  - a[3]*a[10]) + a[8] * (a[2]*a[7]   - a[3]*a[6]);
	inv[11] = - a[0] * (a[5]*a[11]  - a[7]*a[9]) + a[4] * (a[1]*a[11]  - a[3]*a[9]) - a[8] * (a[1]*a[7]   - a[3]*a[5]);
	inv[15] =   a[0] * (a[5]*a[10]  - a[6]*a[9]) - a[4] * (a[1]*a[10]  - a[2]*a[9]) + a[8] * (a[1]*a[6]   - a[2]*a[5]);

	// determinant
	T det = a[0]*inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
	if ( std::fabs(det) < 1e-12f ) return m; // singular

	T invDet = 1 / det;
	for (int i = 0; i < 16; ++i ) inv[i] *= invDet;

	return inv;
}
template<typename T> pod::Vector3t<T> uf::matrix::multiply( const pod::Matrix4t<T>& mat, const pod::Vector3t<T>& v, T w, bool div ) {
	auto res4 = uf::matrix::multiply(mat, pod::Vector4t<T>{ v[0], v[1], v[2], w }, div);
	return pod::Vector3t<T>{ res4[0], res4[1], res4[2] };
}
template<typename T>
pod::Vector2t<T> uf::matrix::multiply(const pod::Matrix2t<T>& mat, const pod::Vector2t<T>& v ) {
	return pod::Vector2t<T>{
		v[0] * mat(0,0) + v[1] * mat(0,1),
		v[0] * mat(1,0) + v[1] * mat(1,1)
	};
}

template<typename T>
pod::Vector3t<T> uf::matrix::multiply(const pod::Matrix3t<T>& mat, const pod::Vector3t<T>& v ) {
	return pod::Vector3t<T>{
		v[0] * mat(0,0) + v[1] * mat(0,1) + v[2] * mat(0,2),
		v[0] * mat(1,0) + v[1] * mat(1,1) + v[2] * mat(1,2),
		v[0] * mat(2,0) + v[1] * mat(2,1) + v[2] * mat(2,2)
	};
}
template<typename T> pod::Vector4t<T> uf::matrix::multiply( const pod::Matrix4t<T>& mat, const pod::Vector4t<T>& v, bool div ) {
#if UF_USE_SIMD
	if constexpr (std::is_same_v<T,float>) {
		pod::Vector4t<T> res = uf::simd::matMult( mat, v );
		if ( div && res.w > 0 ) res /= res.w;
		return res;
	}
#endif
#if UF_ENV_DREAMCAST
	MATH_Load_XMTRX( (ALL_FLOATS_STRUCT*) &mat[0] );
	auto t = MATH_Matrix_Transform( v[0], v[1], v[2], v[3] );
	auto res = *((pod::Vector4t<T>*) &t);
	if ( div && res.w > 0 ) res /= res.w;
	return res;
#else
	auto res = pod::Vector4t<T>{
		v[0] * mat(0,0) + v[1] * mat(0,1) + v[2] * mat(0,2) + v[3] * mat(0,3),
		v[0] * mat(1,0) + v[1] * mat(1,1) + v[2] * mat(1,2) + v[3] * mat(1,3),
		v[0] * mat(2,0) + v[1] * mat(2,1) + v[2] * mat(2,2) + v[3] * mat(2,3),
		v[0] * mat(3,0) + v[1] * mat(3,1) + v[2] * mat(3,2) + v[3] * mat(3,3)
	};
	if ( div && res.w > 0 ) res /= res.w;
	return res;
#endif
}

// functions that serve as the basis to creating SRT matrices, specifically for applying to an identity matrix
template<typename T> T uf::matrix::translate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	T res = matrix;
	res(0,3) = vector.x;
	res(1,3) = vector.y;
	res(2,3) = vector.z;
	return res;
}
template<typename T> T uf::matrix::rotate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	T res = matrix;

	if (vector.x != 0) {
		T Rx = uf::matrix::identity<T>();
		Rx(1,1) = cos(vector.x); Rx(1,2) = -sin(vector.x);
		Rx(2,1) = sin(vector.x); Rx(2,2) = cos(vector.x);
		res = uf::matrix::multiply(res, Rx);
	}
	if (vector.y != 0) {
		T Ry = uf::matrix::identity<T>();
		Ry(0,0) = cos(vector.y); Ry(0,2) = sin(vector.y);
		Ry(2,0) = -sin(vector.y); Ry(2,2) = cos(vector.y);
		res = uf::matrix::multiply(res, Ry);
	}
	if (vector.z != 0) {
		T Rz = uf::matrix::identity<T>();
		Rz(0,0) = cos(vector.z); Rz(0,1) = -sin(vector.z);
		Rz(1,0) = sin(vector.z); Rz(1,1) = cos(vector.z);
		res = uf::matrix::multiply(res, Rz);
	}
	return res;
}
template<typename T> T uf::matrix::scale( const T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	T res = matrix;
	res(0,0) = vector.x;
	res(1,1) = vector.y;
	res(2,2) = vector.z;
	return res;
}
// extract translation from matrix
template<typename T>
pod::Vector3t<typename T::type_t> uf::matrix::extractTranslation( const T& matrix ) {
	return { matrix(0,3), matrix(1,3), matrix(2,3) };
}

// extracts the scale by calculating the length of the 3 basis column vectors
template<typename T>
pod::Vector3t<typename T::type_t> uf::matrix::extractScale( const T& matrix ) {
	using type_t = typename T::type_t;

	type_t sx = std::sqrt( matrix(0,0) * matrix(0,0) + matrix(1,0) * matrix(1,0) + matrix(2,0) * matrix(2,0) );
	type_t sy = std::sqrt( matrix(0,1) * matrix(0,1) + matrix(1,1) * matrix(1,1) + matrix(2,1) * matrix(2,1) );
	type_t sz = std::sqrt( matrix(0,2) * matrix(0,2) + matrix(1,2) * matrix(1,2) + matrix(2,2) * matrix(2,2) );

	// to-do: write uf::matrix::determinant()
	type_t det =  matrix(0,0) * ( matrix(1,1) * matrix(2,2) - matrix(2,1) * matrix(1,2))
				- matrix(0,1) * ( matrix(1,0) * matrix(2,2) - matrix(1,2) * matrix(2,0))
				+ matrix(0,2) * ( matrix(1,0) * matrix(2,1) - matrix(1,1) * matrix(2,0));

	if ( det < 0 ) {
		sx = -sx;
	}
	return { sx, sy, sz };
}

// extracts the rotation by normalizing out the scale
template<typename T>
pod::Vector4t<typename T::type_t> uf::matrix::extractRotation( const T& matrix ) {
	using type_t = typename T::type_t;

	pod::Vector4t<typename T::type_t> q;
	pod::Vector3t<type_t> s = uf::matrix::extractScale( matrix );

	type_t invX = (s.x != 0) ? (1.0 / s.x) : 0;
	type_t invY = (s.y != 0) ? (1.0 / s.y) : 0;
	type_t invZ = (s.z != 0) ? (1.0 / s.z) : 0;

	type_t m00 = matrix(0,0) * invX; type_t m01 = matrix(0,1) * invY; type_t m02 = matrix(0,2) * invZ;
	type_t m10 = matrix(1,0) * invX; type_t m11 = matrix(1,1) * invY; type_t m12 = matrix(1,2) * invZ;
	type_t m20 = matrix(2,0) * invX; type_t m21 = matrix(2,1) * invY; type_t m22 = matrix(2,2) * invZ;

	type_t trace = m00 + m11 + m22;

	if ( trace > 0.0 ) {
		type_t root = std::sqrt(trace + 1.0);
		q.w = 0.5 * root;
		root = 0.5 / root;
		q.x = (m21 - m12) * root;
		q.y = (m02 - m20) * root;
		q.z = (m10 - m01) * root;
	} else {
		int i = 0;
		if ( m11 > m00 ) i = 1;
		if ( m22 > (i == 0 ? m00 : m11) ) i = 2;

		if (i == 0) {
			type_t root = std::sqrt(m00 - m11 - m22 + 1.0);
			q.x = 0.5 * root;
			root = 0.5 / root;
			q.w = (m21 - m12) * root;
			q.y = (m01 + m10) * root;
			q.z = (m02 + m20) * root;
		} else if (i == 1) {
			type_t root = std::sqrt(m11 - m00 - m22 + 1.0);
			q.y = 0.5 * root;
			root = 0.5 / root;
			q.w = (m02 - m20) * root;
			q.x = (m01 + m10) * root;
			q.z = (m12 + m21) * root;
		} else {
			type_t root = std::sqrt(m22 - m00 - m11 + 1.0);
			q.z = 0.5 * root;
			root = 0.5 / root;
			q.w = (m10 - m01) * root;
			q.x = (m02 + m20) * root;
			q.y = (m12 + m21) * root;
		}
	}

	return uf::vector::normalize( q );
}

template<typename T> pod::Matrix<typename T::type_t, T::columns, T::columns> uf::matrix::multiply_( T& left, const T& right ) {
	return left = uf::matrix::multiply((const T&) left, right);
}
template<typename T> T& uf::matrix::translate_( T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	return matrix = uf::matrix::translate((const T&) matrix, vector);
}
template<typename T> T& uf::matrix::rotate_( T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	return matrix = uf::matrix::rotate((const T&) matrix, vector);
}
template<typename T> T& uf::matrix::scale_( T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	return matrix = uf::matrix::scale((const T&) matrix, vector);
}
template<typename T> T& uf::matrix::inverse_( T& matrix ) {
	return matrix = uf::matrix::inverse((const T&) matrix);
}

template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::orthographic( T l, T r, T b, T t, T f, T n ) {
	pod::Matrix4t<T> m = uf::matrix::identity();
	m(0,0) = static_cast<T>(2) / (r - l);
	m(1,1) = static_cast<T>(2) / (t - b);
	m(2,2) = static_cast<T>(-2) / (f - n);

	// Translation terms go in the last column (col = 3)
	m(0,3) = - (r + l) / (r - l);
	m(1,3) = - (t + b) / (t - b);
	m(2,3) = - (f + n) / (f - n);
	return m;
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::orthographic( T l, T r, T b, T t ) {
	pod::Matrix4t<T> m = uf::matrix::identity();
	m(0,0) = static_cast<T>(2) / (r - l);
	m(1,1) = static_cast<T>(2) / (t - b);
	m(2,2) = static_cast<T>(1);

	m(0,3) = - (r + l) / (r - l);
	m(1,3) = - (t + b) / (t - b);
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::perspective( T fov, T raidou, T znear, T zfar ) {
    pod::Matrix4t<T> m = uf::matrix::identity<T>();

	T f = static_cast<T>(1) / tan(static_cast<T>(0.5) * fov);
    m(0,0) = f / raidou;
    m(1,1) = f;
#if UF_USE_VULKAN
    m(1,1) = -f;
#endif
    m(3,2) = 1;
    m(3,3) = 0;

    if ( zfar <= 0 ) {
        m(2,2) = 0;
        m(2,3) = znear;
    } else {
        T range = zfar - znear;
        m(2,2) = zfar / range;
        m(2,3) = -(zfar * znear) / range;
    }

    return m;
}
template<typename T> T& uf::matrix::copy( T& destination, const T& source ) {
	FOR_EACH(T::rows * T::columns, {
		destination[i] = source[i];
	});
	return destination;
}
template<typename T> T& uf::matrix::copy( T& destination, typename T::type_t* const source ) {
	FOR_EACH(T::rows * T::columns, {
		destination[i] = source[i];
	});
	return destination;
}

template<typename T> pod::Vector3t<typename T::type_t> /*UF_API*/ uf::matrix::eulerAngles( const T& M ) {
	typename T::type_t T1 = atan2(M[2*4+1], M[2*4+2]);
	typename T::type_t C2 = sqrt(M[0*4+0]*M[0*4+0] + M[1*4+0]*M[1*4+0]);
	typename T::type_t T2 = atan2(-M[2*4+0], C2);
	typename T::type_t S1 = sin(T1);
	typename T::type_t C1 = cos(T1);
	typename T::type_t T3 = atan2(S1*M[0*4+2] - C1*M[0*4+1], C1*M[1*4+1] - S1*M[1*4+2  ]);
	return pod::Vector3t<typename T::type_t>{-T1, -T2, -T3};
}


template<typename T, size_t R, size_t C>
ext::json::Value /*UF_API*/ uf::matrix::encode( const pod::Matrix<T,R,C>& m, const ext::json::EncodingSettings& settings ) {
	ext::json::Value json;
	if ( settings.quantize )
		#pragma unroll // GCC unroll R*C
		for ( auto i = 0; i < R*C; ++i )
			json[i] = uf::math::quantizeShort( m[i] );
	else
		#pragma unroll // GCC unroll R*C
		for ( auto i = 0; i < R*C; ++i )
			json[i] = m[i];

	return json;
}
template<typename T, size_t R, size_t C>
pod::Matrix<T,R,C>& /*UF_API*/ uf::matrix::decode( const ext::json::Value& json, pod::Matrix<T,R,C>& m ) {
	if ( ext::json::isArray(json) )
		#pragma unroll // GCC unroll T::size
		for ( auto i = 0; i < R*C && i < json.size(); ++i )
			m[i] = json[i].as<T>(m[i]);
	else if ( ext::json::isObject(json) ) {
		auto i = 0;
		ext::json::forEach(json, [&](const ext::json::Value& c){
			if ( i >= R*C ) return;
			m[i] = c.as<T>(m[i]);
			++i;
		});
	}
	return m;
}

template<typename T, size_t R, size_t C>
pod::Matrix<T,R,C> /*UF_API*/ uf::matrix::decode( const ext::json::Value& json, const pod::Matrix<T,R,C>& _m ) {
	pod::Matrix<T,R,C> m = _m;
	if ( ext::json::isArray(json) )
		#pragma unroll // GCC unroll T::size
		for ( auto i = 0; i < R*C && i < json.size(); ++i )
			m[i] = json[i].as<T>(_m[i]);
	else if ( ext::json::isObject(json) ) {
		auto i = 0;
		ext::json::forEach(json, [&](const ext::json::Value& c){
			if ( i >= R*C ) return;
			m[i] = c.as<T>(_m[i]);
			++i;
		});
	}
	return m;
}

template<typename T, size_t R, size_t C>
uf::stl::string /*UF_API*/ uf::matrix::toString( const pod::Matrix<T,R,C>& m ) {
	uf::stl::stringstream ss;
	ss << "Matrix(\n\t";
	#pragma unroll // GCC unroll C
	for ( auto c = 0; c < C; ++c ) {
		#pragma unroll // GCC unroll R
		for ( auto r = 0; r < R; ++r ) {
			ss << m[r+c*C] << ", ";
		}
		if ( c + 1 < C ) ss << "\n\t";
	}
	ss << "\n)";
	return ss.str();
}