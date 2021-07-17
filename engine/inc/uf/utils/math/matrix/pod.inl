#if !__clang__ && __GNUC__
	#pragma GCC push_options
	#pragma GCC optimize ("unroll-loops")
#endif

// 	Overloaded ops
// Accessing via subscripts
template<typename T, size_t R, size_t C>
inline T& pod::Matrix<T,R,C>::operator[](uint_fast8_t i) {
//	static T null = 0.0/0.0;
//	if ( i >= R*C ) return null;
	return this->components[i];
}
template<typename T, size_t R, size_t C>
inline const T& pod::Matrix<T,R,C>::operator[](uint_fast8_t i) const {
//	static T null = 0.0/0.0;
//	if ( i >= R*C ) return null;
	return this->components[i];
}
template<typename T, size_t R, size_t C>
pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator()() const {
	pod::Matrix<T,R,C> matrix;
	#pragma unroll // GCC unroll C
	for ( uint_fast8_t c = 0; c < C; ++c ) 
		#pragma unroll // GCC unroll R
		for ( uint_fast8_t r = 0; r < R; ++r ) 
			matrix[r+c*C] = (r == c ? 1 : 0);
	return matrix;
}
template<typename T, size_t R, size_t C>
inline T& pod::Matrix<T,R,C>::operator()(uint_fast8_t r, uint_fast8_t c) {
	return this->components[r+c*C];
}
template<typename T, size_t R, size_t C>
inline const T& pod::Matrix<T,R,C>::operator()(uint_fast8_t r, uint_fast8_t c) const {
	return this->components[r+c*C];
}
/*
template<typename T, size_t R, size_t C>
T* pod::Matrix<T,R,C>::operator[](size_t i) {
	return this->components[i];
}
template<typename T, size_t R, size_t C>
const T* pod::Matrix<T,R,C>::operator[](size_t i) const {
	return this->components[i];
}
*/
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::identity() {
	alignas(16) pod::Matrix4t<T> matrix;
	#pragma unroll // GCC unroll 4
	for ( uint_fast8_t c = 0; c < 4; ++c ) 
		#pragma unroll // GCC unroll 4
		for ( uint_fast8_t r = 0; r < 4; ++r ) 
			matrix[r+c*4] = (r == c ? 1 : 0);
	return matrix;
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::initialize( const T* list ) {
	alignas(16) pod::Matrix4t<T> matrix;
//	memcpy(&matrix[0], list, sizeof(matrix));
	#pragma unroll // GCC unroll 16
	for ( uint_fast8_t i = 0; i < 16; ++i )
		matrix.components[i] = list[i];

/*
	for ( uint_fast8_t r = 0; r < 4; ++r ) 
		for ( uint_fast8_t c = 0; c < 4; ++c ) 
			matrix[r+c*4] = list[r+c*4];
*/
	return matrix;
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::initialize( const uf::stl::vector<T>& list ) {
	alignas(16) pod::Matrix4t<T> matrix;
	if ( list.size() != 16 ) return matrix;
//	memcpy(&matrix[0], &list[0], sizeof(matrix));
	#pragma unroll // GCC unroll 16
	for ( uint_fast8_t i = 0; i < 16; ++i )
		matrix.components[i] = list[i];

/*
	#pragma unroll // GCC unroll 4
	for ( uint_fast8_t r = 0; r < 4; ++r ) 
		#pragma unroll // GCC unroll 4
		for ( uint_fast8_t c = 0; c < 4; ++c ) 
			matrix[r+c*4] = list[r+c*4];
*/
	return matrix;
}
template<typename T> pod::Matrix<typename T::type_t, T::columns, T::columns> uf::matrix::identityi(){
	alignas(16) pod::Matrix<typename T::type_t, T::columns, T::columns> matrix;

	#pragma unroll // GCC unroll T::columns
	for ( uint_fast8_t c = 0; c < T::columns; ++c ) 
		#pragma unroll // GCC unroll T::rows
		for ( uint_fast8_t r = 0; r < T::rows; ++r ) 
			matrix[r+c*T::columns] = (r == c ? 1 : 0);


	return matrix;
}
// Arithmetic
// 	Negation
template<typename T, size_t R, size_t C>
inline pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator-() const {
	return uf::matrix::inverse(*this);
}
// 	Multiplication between two matrices
template<typename T, size_t R, size_t C>
inline pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator*( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::multiply(*this, matrix);
}
// 	Multiplication between two matrices
template<typename T, size_t R, size_t C>
inline pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator*( T scalar ) const {
	return uf::matrix::multiplyAll(*this, scalar);
}
// 	Multiplication between two matrices
template<typename T, size_t R, size_t C>
inline pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator+( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::add(*this, matrix);
}
// 	Multiplication set between two matrices
template<typename T, size_t R, size_t C>
inline pod::Matrix<T,R,C>& pod::Matrix<T,R,C>::operator *=( const Matrix<T,R,C>& matrix ) {
	return uf::matrix::multiply(*this, matrix);
}
// 	Equality check between two matrices (equals)
template<typename T, size_t R, size_t C>
inline bool pod::Matrix<T,R,C>::operator==( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::equals( *this, matrix );
}
// 	Equality check between two matrices (not equals)
template<typename T, size_t R, size_t C>
inline bool pod::Matrix<T,R,C>::operator!=( const Matrix<T,R,C>& matrix ) const {
	return !uf::matrix::equals( *this, matrix );
}

// 	Equality checking
// 	Equality check between two matrices (less than)
template<typename T> int uf::matrix::compareTo( const T& left, const T& right ) {
	return memcmp( &left[0], &right[0], sizeof(left) );
}
// 	Equality check between two matrices (equals)
template<typename T> bool uf::matrix::equals( const T& left, const T& right ) {
	return uf::matrix::compareTo(left, right) == 0;
}
// 	Basic arithmetic
// 	Multiplies two matrices of same type and size together
template<typename T> pod::Matrix<T,4,4> uf::matrix::multiply( const pod::Matrix<T,4,4>& left, const pod::Matrix<T,4,4>& right ) {
	alignas(16) pod::Matrix<T,4,4> res;
#if UF_USE_SIMD
	auto row1 = uf::simd::load(&left[0]);
	auto row2 = uf::simd::load(&left[4]);
	auto row3 = uf::simd::load(&left[8]);
	auto row4 = uf::simd::load(&left[12]);
	#pragma unroll // GCC unroll 4
	for( uint_fast8_t i = 0; i < 4; i++) {
		auto brod1 = uf::simd::set(right[4*i + 0]);
		auto brod2 = uf::simd::set(right[4*i + 1]);
		auto brod3 = uf::simd::set(right[4*i + 2]);
		auto brod4 = uf::simd::set(right[4*i + 3]);
		auto row = uf::simd::add(
					uf::simd::add(
						uf::simd::mul(brod1, row1),
						uf::simd::mul(brod2, row2)),
					uf::simd::add(
						uf::simd::mul(brod3, row3),
						uf::simd::mul(brod4, row4)));
		uf::simd::store(row, &res[4*i]);
	}

	return res;
#elif UF_ENV_DREAMCAST
// 	kallistios has dedicated SH4 asm for these or something
	mat_load( (matrix_t*) &left[0] );
	mat_apply( (matrix_t*) &right[0] );
	mat_store( (matrix_t*) &res[0]);

// 	gives very wrong output, not sure why
//	MATH_Load_Matrix_Product( (ALL_FLOATS_STRUCT*) &left[0], (ALL_FLOATS_STRUCT*) &right[0] );
//	MATH_Store_XMTRX( (ALL_FLOATS_STRUCT*) &res[0]);
	return res;
#elif 0
	// 
	float* dstPtr = &res[0];
	const float* leftPtr = &right[0];

	#pragma unroll // GCC unroll 4
	for (uint_fast8_t i = 0; i < 4; ++i) {
		#pragma unroll // GCC unroll 4
		for (uint_fast8_t j = 0; j < 4; ++j) {
			const float* rightPtr = &left[0] + j;

			float sum = leftPtr[0] * rightPtr[0];
			#pragma unroll // GCC unroll 3
			for (uint_fast8_t n = 1; n < 4; ++n) {
				rightPtr += 4;
				sum += leftPtr[n] * rightPtr[0];
			}
			*dstPtr++ = sum;
		}
		leftPtr += 4;
	}
	return res;
#elif 0
	// don't know if it's more performant than below
	uint_fast8_t i = 0;

	#pragma unroll // GCC unroll 4
	for ( uint_fast8_t c = 0; c < 4; c++ ) {
		#pragma unroll // GCC unroll 4
		for ( uint_fast8_t r = 0; r < 4; r++ ) {
			res[i++] = uf::vector::dot( { right[0+c*4], right[1+c*4], right[2+c*4], right[3+c*4] }, { left[r+0*4], left[r+1*4], left[r+2*4], left[r+3*4] } );
		}
	}

	return res;
#else
	// it works
	const pod::Vector<T,4>& srcA0 = *((pod::Vector<T,4>*) &left[0]);
	const pod::Vector<T,4>& srcA1 = *((pod::Vector<T,4>*) &left[4]);
	const pod::Vector<T,4>& srcA2 = *((pod::Vector<T,4>*) &left[8]);
	const pod::Vector<T,4>& srcA3 = *((pod::Vector<T,4>*) &left[12]);

	const pod::Vector<T,4>& srcB0 = *((pod::Vector<T,4>*) &right[0]);
	const pod::Vector<T,4>& srcB1 = *((pod::Vector<T,4>*) &right[4]);
	const pod::Vector<T,4>& srcB2 = *((pod::Vector<T,4>*) &right[8]);
	const pod::Vector<T,4>& srcB3 = *((pod::Vector<T,4>*) &right[12]);
	
	pod::Vector<T,4>& dst0 = *((pod::Vector<T,4>*) &res[0]);
	pod::Vector<T,4>& dst1 = *((pod::Vector<T,4>*) &res[4]);
	pod::Vector<T,4>& dst2 = *((pod::Vector<T,4>*) &res[8]);
	pod::Vector<T,4>& dst3 = *((pod::Vector<T,4>*) &res[12]);

	dst0 = srcA0 * srcB0[0] + srcA1 * srcB0[1] + srcA2 * srcB0[2] + srcA3 * srcB0[3];
	dst1 = srcA0 * srcB1[0] + srcA1 * srcB1[1] + srcA2 * srcB1[2] + srcA3 * srcB1[3];
	dst2 = srcA0 * srcB2[0] + srcA1 * srcB2[1] + srcA2 * srcB2[2] + srcA3 * srcB2[3];
	dst3 = srcA0 * srcB3[0] + srcA1 * srcB3[1] + srcA2 * srcB3[2] + srcA3 * srcB3[3];
	return res;
#endif
}
template<typename T, typename U> pod::Matrix<typename T::type_t, T::columns, T::columns> uf::matrix::multiply( const T& left, const U& right ) {
	alignas(16) pod::Matrix<typename T::type_t,T::rows,T::columns> res;
#if 1
	float* dstPtr = &res[0];
	const float* leftPtr = &right[0];

	#pragma unroll // GCC unroll T::rows
	for (uint_fast8_t i = 0; i < T::rows; ++i) {
		#pragma unroll // GCC unroll T::columns
		for (uint_fast8_t j = 0; j < T::columns; ++j) {
			const float* rightPtr = &left[0] + j;

			float sum = leftPtr[0] * rightPtr[0];
			#pragma unroll // GCC unroll T::columns - 1
			for (uint_fast8_t n = 1; n < T::columns; ++n) {
				rightPtr += T::columns;
				sum += leftPtr[n] * rightPtr[0];
			}
			*dstPtr++ = sum;
		}
		leftPtr += T::columns;
	}

#else
	uint_fast8_t i = 0;
	#pragma unroll // GCC unroll
	for ( uint_fast8_t col = 0; col < R; col++ ) {
		#pragma unroll // GCC unroll
		for ( uint_fast8_t row = 0; row < C; row++ ) {
			auto& sum = res[i++];
			#pragma unroll // GCC unroll
			for ( uint_fast8_t i = 0; i < C; i++ )
				sum += right[i + col * C] * left[row + i * R];
		}
	}
#endif
	return res;
}
template<typename T> T /*UF_API*/ uf::matrix::multiplyAll( const T& m, typename T::type_t scalar ) {
	alignas(16) T matrix;
	#pragma unroll // GCC unroll T::rows * T::columns
	for ( uint_fast8_t i = 0; i < T::rows * T::columns; ++i )
		matrix[i] = m[i] * scalar;

	return matrix;
}
template<typename T> T /*UF_API*/ uf::matrix::add( const T& lhs, const T& rhs ) {
	alignas(16) T matrix;
	#pragma unroll // GCC unroll T::rows * T::columns
	for ( uint_fast8_t i = 0; i < T::rows * T::columns; ++i )
		matrix[i] = lhs[i] + rhs[i];

	return matrix;
}
// 	Transpose matrix
template<typename T> T uf::matrix::transpose( const T& matrix ) {
	alignas(16) T transpose;

	#pragma unroll // GCC unroll T::rows
	for ( typename T::type_t r = 0; r < T::rows; ++r )
		#pragma unroll // GCC unroll T::columns
		for ( typename T::type_t c = 0; c < T::columns; ++c )
			transpose[c * T::rows + r] = matrix[r * T::columns + c];


	return transpose;
}
// 	Flip sign of all components
template<typename T> T uf::matrix::inverse( const T& matrix ) {
	if ( T::rows != 4 || T::columns != 4 ) return matrix;

	const typename T::type_t* m = &matrix[0];
	alignas(16) typename T::type_t inv[16];
	typename T::type_t det;
	uint_fast8_t i;

	inv[0] = m[5]  * m[10] * m[15] - 
			 m[5]  * m[11] * m[14] - 
			 m[9]  * m[6]  * m[15] + 
			 m[9]  * m[7]  * m[14] +
			 m[13] * m[6]  * m[11] - 
			 m[13] * m[7]  * m[10];
	inv[4] = -m[4]  * m[10] * m[15] + 
			  m[4]  * m[11] * m[14] + 
			  m[8]  * m[6]  * m[15] - 
			  m[8]  * m[7]  * m[14] - 
			  m[12] * m[6]  * m[11] + 
			  m[12] * m[7]  * m[10];
	inv[8] = m[4]  * m[9] * m[15] - 
			 m[4]  * m[11] * m[13] - 
			 m[8]  * m[5] * m[15] + 
			 m[8]  * m[7] * m[13] + 
			 m[12] * m[5] * m[11] - 
			 m[12] * m[7] * m[9];
	inv[12] = -m[4]  * m[9] * m[14] + 
			   m[4]  * m[10] * m[13] +
			   m[8]  * m[5] * m[14] - 
			   m[8]  * m[6] * m[13] - 
			   m[12] * m[5] * m[10] + 
			   m[12] * m[6] * m[9];
	inv[1] = -m[1]  * m[10] * m[15] + 
			  m[1]  * m[11] * m[14] + 
			  m[9]  * m[2] * m[15] - 
			  m[9]  * m[3] * m[14] - 
			  m[13] * m[2] * m[11] + 
			  m[13] * m[3] * m[10];
	inv[5] = m[0]  * m[10] * m[15] - 
			 m[0]  * m[11] * m[14] - 
			 m[8]  * m[2] * m[15] + 
			 m[8]  * m[3] * m[14] + 
			 m[12] * m[2] * m[11] - 
			 m[12] * m[3] * m[10];
	inv[9] = -m[0]  * m[9] * m[15] + 
			  m[0]  * m[11] * m[13] + 
			  m[8]  * m[1] * m[15] - 
			  m[8]  * m[3] * m[13] - 
			  m[12] * m[1] * m[11] + 
			  m[12] * m[3] * m[9];
	inv[13] = m[0]  * m[9] * m[14] - 
			  m[0]  * m[10] * m[13] - 
			  m[8]  * m[1] * m[14] + 
			  m[8]  * m[2] * m[13] + 
			  m[12] * m[1] * m[10] - 
			  m[12] * m[2] * m[9];
	inv[2] = m[1]  * m[6] * m[15] - 
			 m[1]  * m[7] * m[14] - 
			 m[5]  * m[2] * m[15] + 
			 m[5]  * m[3] * m[14] + 
			 m[13] * m[2] * m[7] - 
			 m[13] * m[3] * m[6];
	inv[6] = -m[0]  * m[6] * m[15] + 
			  m[0]  * m[7] * m[14] + 
			  m[4]  * m[2] * m[15] - 
			  m[4]  * m[3] * m[14] - 
			  m[12] * m[2] * m[7] + 
			  m[12] * m[3] * m[6];
	inv[10] = m[0]  * m[5] * m[15] - 
			  m[0]  * m[7] * m[13] - 
			  m[4]  * m[1] * m[15] + 
			  m[4]  * m[3] * m[13] + 
			  m[12] * m[1] * m[7] - 
			  m[12] * m[3] * m[5];
	inv[14] = -m[0]  * m[5] * m[14] + 
			   m[0]  * m[6] * m[13] + 
			   m[4]  * m[1] * m[14] - 
			   m[4]  * m[2] * m[13] - 
			   m[12] * m[1] * m[6] + 
			   m[12] * m[2] * m[5];
	inv[3] = -m[1] * m[6] * m[11] + 
			  m[1] * m[7] * m[10] + 
			  m[5] * m[2] * m[11] - 
			  m[5] * m[3] * m[10] - 
			  m[9] * m[2] * m[7] + 
			  m[9] * m[3] * m[6];
	inv[7] = m[0] * m[6] * m[11] - 
			 m[0] * m[7] * m[10] - 
			 m[4] * m[2] * m[11] + 
			 m[4] * m[3] * m[10] + 
			 m[8] * m[2] * m[7] - 
			 m[8] * m[3] * m[6];
	inv[11] = -m[0] * m[5] * m[11] + 
			   m[0] * m[7] * m[9] + 
			   m[4] * m[1] * m[11] - 
			   m[4] * m[3] * m[9] - 
			   m[8] * m[1] * m[7] + 
			   m[8] * m[3] * m[5];
	inv[15] = m[0] * m[5] * m[10] - 
			  m[0] * m[6] * m[9] - 
			  m[4] * m[1] * m[10] + 
			  m[4] * m[2] * m[9] + 
			  m[8] * m[1] * m[6] - 
			  m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	if (det == 0) return matrix;
	det = 1.0 / det;
	alignas(16) T inverted;
	#pragma unroll // GCC unroll 16
	for ( i = 0; i < 16; ++i )
		inverted[i] = inv[i] * det;

	return inverted;
}
// 	Writes to first value
// 	Multiplies two matrices of same type and size together
template<typename T> pod::Matrix<typename T::type_t, T::columns, T::columns> uf::matrix::multiply( T& left, const T& right ) {
	return left = uf::matrix::multiply((const T&) left, right);
}
template<typename T> pod::Vector3t<T> uf::matrix::multiply( const pod::Matrix4t<T>& mat, const pod::Vector3t<T>& vector, T w, bool div ) {
	return uf::matrix::multiply( mat, pod::Vector4t<T>{ vector[0], vector[1], vector[2], w }, div );
}
template<typename T> pod::Vector4t<T> uf::matrix::multiply( const pod::Matrix4t<T>& mat, const pod::Vector4t<T>& vector, bool div ) {
#if UF_ENV_DREAMCAST
	MATH_Load_XMTRX( (ALL_FLOATS_STRUCT*) &mat[0] );
	auto t = MATH_Matrix_Transform( vector[0], vector[1], vector[2], vector[3] );
	auto res = *((pod::Vector4t<T>*) &t);
	if ( div && res.w > 0 ) res /= res.w;
	return res;
#else
	alignas(16) auto res = pod::Vector4t<T>{
		vector[0] * mat[0] + vector[1] * mat[4] + vector[2] * mat[8] + vector[3] * mat[12],
		vector[0] * mat[1] + vector[1] * mat[5] + vector[2] * mat[9] + vector[3] * mat[13],
		vector[0] * mat[2] + vector[1] * mat[6] + vector[2] * mat[10] + vector[3] * mat[14],
		vector[0] * mat[3] + vector[1] * mat[7] + vector[2] * mat[11] + vector[3] * mat[15]
	};
	if ( div && res.w > 0 ) res /= res.w;
	return res;
#endif
}
// 	Flip sign of all components
template<typename T> T& uf::matrix::invert( T& matrix ) {
	return matrix = uf::matrix::inverse((const T&) matrix);
}
// 	Complex arithmetic
template<typename T> T uf::matrix::translate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	alignas(16) T res = matrix;
	res[12] = vector.x;
	res[13] = vector.y;
	res[14] = vector.z;
	return res;
}
template<typename T> T uf::matrix::rotate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	alignas(16) T res = matrix;
	if ( vector.x != 0 ) {	
		res[5] = cos( vector.x );
		res[6] = sin( vector.x );
		res[9] = -1 * sin( vector.x );
		res[10] = cos( vector.x );
	}
	
	if ( vector.y != 0 ) {	
		res[0] = cos( vector.y );
		res[2] = -1 * sin( vector.y );
		res[8] = sin( vector.y );
		res[10] = cos( vector.y );
	}

	if ( vector.z != 0 ) {	
		res[0] = cos( vector.z );
		res[1] = sin( vector.z );
		res[4] = -1 * sin( vector.z );
		res[5] = cos( vector.z );
	}
	return res;
}
template<typename T> T uf::matrix::scale( const T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	alignas(16) T res = matrix;
	res[0] = vector.x;
	res[5] = vector.y;
	res[10] = vector.z;
	return res;
}
template<typename T> T& uf::matrix::translate( T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	matrix[12] = vector.x;
	matrix[13] = vector.y;
	matrix[14] = vector.z;
	return matrix;
}
template<typename T> T& uf::matrix::rotate( T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	if ( vector.x != 0 ) {	
		matrix[5] = cos( vector.x );
		matrix[6] = sin( vector.x );
		matrix[9] = -1 * sin( vector.x );
		matrix[10] = cos( vector.x );
	}
	
	if ( vector.y != 0 ) {	
		matrix[0] = cos( vector.y );
		matrix[2] = -1 * sin( vector.y );
		matrix[8] = sin( vector.y );
		matrix[10] = cos( vector.y );
	}

	if ( vector.z != 0 ) {	
		matrix[0] = cos( vector.z );
		matrix[1] = sin( vector.z );
		matrix[4] = -1 * sin( vector.z );
		matrix[5] = cos( vector.z );
	}
	return matrix;
}
template<typename T> T& uf::matrix::scale( T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	matrix[0] = vector.x;
	matrix[5] = vector.y;
	matrix[10] = vector.z;
	return matrix;
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::orthographic( T l, T r, T b, T t, T f, T n ) {
	alignas(16) pod::Matrix4t<T> m = uf::matrix::identity();
	m[0*4+0] = 2 / (r - l);
    m[1*4+1] = 2 / (t - b);
    m[2*4+2] = - 2 / (f - n);
    m[3*4+0] = - (r + l) / (r - l);
    m[3*4+1] = - (t + b) / (t - b);
    m[3*4+2] = - (f + n) / (f - n);
	return m;
/*
	uf::stl::vector<T> m = {
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, -2 / (f - n), 0,
		-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1,
	};
	return uf::matrix::initialize(m);
*/
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::orthographic( T l, T r, T b, T t ) {
	return pod::Matrix4t<T>({
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, 1, 0,
		-(r + l) / (r - l), -(t+b)/(t-b), 0, 1
	});
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::perspective( T fov, T raidou, T znear, T zfar ) {
	if ( uf::matrix::reverseInfiniteProjection ) {
		T f = static_cast<T>(1) / tan( static_cast<T>(0.5) * fov );
	#if UF_USE_OPENGL
		return pod::Matrix4t<T>({
			f / raidou, 	0, 	 	0, 		0,
			0, 			 	f, 	 	0, 		0,
			0,       		0,    	0, 		1,
			0,       		0,   znear, 	0
		});
	#elif UF_USE_VULKAN
		return pod::Matrix4t<T>({
			f / raidou, 	0, 	 	0, 		0,
			0, 				-f, 	0, 		0,
			0,       		0,    	0, 		1,
			0,       		0,   znear, 	0
		});
	#endif
	} else {
		T range = znear - zfar;
		T f = tan( static_cast<T>(0.5) * fov );

		T Sx = static_cast<T>(1) / (f * raidou);
		T Sy = static_cast<T>(1) / f;
		T Sz = (-znear - zfar) / range;
		T Pz = static_cast<T>(2) * zfar * znear / range;
	#if UF_USE_VULKAN
		Sy = -Sy;
	#endif
		return pod::Matrix4t<T>({
			Sx, 	 0, 	 0, 	  0,
			 0, 	Sy, 	 0, 	  0,
			 0, 	 0, 	Sz, 	  1,
			 0, 	 0, 	Pz, 	  0
		});
	}
}
template<typename T> T& uf::matrix::copy( T& destination, const T& source ) {
	#pragma unroll // GCC unroll 16
	for ( uint_fast8_t i = 0; i < 16; ++i )
		destination[i] = source[i];

	return destination;
}
template<typename T> T& uf::matrix::copy( T& destination, typename T::type_t* const source ) {
	#pragma unroll // GCC unroll 16
	for ( uint_fast8_t i = 0; i < 16; ++i )
		destination[i] = source[i];

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
		for ( uint_fast8_t i = 0; i < R*C; ++i )
			json[i] = uf::math::quantizeShort( m[i] );
	else
		#pragma unroll // GCC unroll R*C
		for ( uint_fast8_t i = 0; i < R*C; ++i )
			json[i] = m[i];

	return json;
}
template<typename T, size_t R, size_t C>
pod::Matrix<T,R,C>& /*UF_API*/ uf::matrix::decode( const ext::json::Value& json, pod::Matrix<T,R,C>& m ) {
	if ( ext::json::isArray(json) )
		#pragma unroll // GCC unroll T::size
		for ( uint_fast8_t i = 0; i < R*C; ++i )
			m[i] = json[i].as<T>(m[i]);
	else if ( ext::json::isObject(json) ) {
		uint_fast8_t i = 0;
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
	alignas(16) pod::Matrix<T,R,C> m = _m;
	if ( ext::json::isArray(json) )
		#pragma unroll // GCC unroll T::size
		for ( uint_fast8_t i = 0; i < R*C; ++i )
			m[i] = json[i].as<T>(_m[i]);
	else if ( ext::json::isObject(json) ) {
		uint_fast8_t i = 0;
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
	for ( uint_fast8_t c = 0; c < C; ++c ) {
		#pragma unroll // GCC unroll R
		for ( uint_fast8_t r = 0; r < R; ++r ) {
			ss << m[r+c*C] << ", ";
		}
		if ( c + 1 < C ) ss << "\n\t";
	}
	ss << "\n)";
	return ss.str();
}

#if !__clang__ && __GNUC__
	#pragma GCC pop_options
#endif