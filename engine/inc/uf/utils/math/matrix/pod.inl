// 	Overloaded ops
// Accessing via subscripts
template<typename T, std::size_t R, std::size_t C>
inline T& pod::Matrix<T,R,C>::operator[](std::size_t i) {
//	static T null = 0.0/0.0;
//	if ( i >= R*C ) return null;
	return this->components[i];
}
template<typename T, std::size_t R, std::size_t C>
inline const T& pod::Matrix<T,R,C>::operator[](std::size_t i) const {
//	static T null = 0.0/0.0;
//	if ( i >= R*C ) return null;
	return this->components[i];
}
template<typename T, std::size_t R, std::size_t C>
pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator()() const {
	pod::Matrix<T,R,C> matrix;
	for ( std::size_t r = 0; r < R; ++r ) 
		for ( std::size_t c = 0; c < C; ++c ) 
			matrix[r+c*C] = (r == c ? 1 : 0);
	return matrix;
}
template<typename T, std::size_t R, std::size_t C>
inline T& pod::Matrix<T,R,C>::operator()(size_t r, size_t c) {
	return this->components[r+c*C];
}
template<typename T, std::size_t R, std::size_t C>
inline const T& pod::Matrix<T,R,C>::operator()(size_t r, size_t c) const {
	return this->components[r+c*C];
}
/*
template<typename T, std::size_t R, std::size_t C>
T* pod::Matrix<T,R,C>::operator[](std::size_t i) {
	return this->components[i];
}
template<typename T, std::size_t R, std::size_t C>
const T* pod::Matrix<T,R,C>::operator[](std::size_t i) const {
	return this->components[i];
}
*/
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::identity() {
	pod::Matrix4t<T> matrix;
	for ( std::size_t r = 0; r < 4; ++r ) 
		for ( std::size_t c = 0; c < 4; ++c ) 
			matrix[r+c*4] = (r == c ? 1 : 0);
	return matrix;
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::ortho( T l, T r, T b, T t, T f, T n ) {
	std::vector<T> m = {
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, -2 / (f - n), 0,
		-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1,
	};
	return uf::matrix::initialize(m);
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::ortho( T l, T r, T b, T t ) {
	std::vector<T> m = {
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, 1, 0,
		-(r + l) / (r - l), -(t+b)/(t-b), 0, 1
	};
	return uf::matrix::initialize(m);
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::initialize( const T* list ) {
	pod::Matrix4t<T> matrix;
//	memcpy(&matrix[0], list, 16);
	for ( std::size_t i = 0; i < 16; ++i )
		matrix.components[i] = list[i];
/*
	for ( std::size_t r = 0; r < 4; ++r ) 
		for ( std::size_t c = 0; c < 4; ++c ) 
			matrix[r+c*4] = list[r+c*4];
*/
	return matrix;
}
template<typename T>
pod::Matrix4t<T> /*UF_API*/ uf::matrix::initialize( const std::vector<T>& list ) {
	pod::Matrix4t<T> matrix;
	if ( list.size() != 16 ) return matrix;
//	memcpy(&matrix[0], &list[0], 16);
	for ( std::size_t i = 0; i < 16; ++i )
		matrix.components[i] = list[i];
/*
	for ( std::size_t r = 0; r < 4; ++r ) 
		for ( std::size_t c = 0; c < 4; ++c ) 
			matrix[r+c*4] = list[r+c*4];
*/
	return matrix;
}
template<typename T> pod::Matrix<typename T::type_t, T::columns, T::columns> uf::matrix::identityi(){
	pod::Matrix<typename T::type_t, T::columns, T::columns> matrix;
	for ( std::size_t r = 0; r < T::columns; ++r ) 
		for ( std::size_t c = 0; c < T::columns; ++c ) 
			matrix[r+c*T::columns] = (r == c ? 1 : 0);
	return matrix;
}
// Arithmetic
// 	Negation
template<typename T, std::size_t R, std::size_t C>
inline pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator-() const {
	return uf::matrix::inverse(*this);
}
// 	Multiplication between two matrices
template<typename T, std::size_t R, std::size_t C>
inline pod::Matrix<T,R,C> pod::Matrix<T,R,C>::operator*( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::multiply(*this, matrix);
}
// 	Multiplication set between two matrices
template<typename T, std::size_t R, std::size_t C>
inline pod::Matrix<T,R,C>& pod::Matrix<T,R,C>::operator *=( const Matrix<T,R,C>& matrix ) {
	return uf::matrix::multiply(*this, matrix);
}
// 	Equality check between two matrices (equals)
template<typename T, std::size_t R, std::size_t C>
inline bool pod::Matrix<T,R,C>::operator==( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::equals( *this, matrix );
}
// 	Equality check between two matrices (not equals)
template<typename T, std::size_t R, std::size_t C>
inline bool pod::Matrix<T,R,C>::operator!=( const Matrix<T,R,C>& matrix ) const {
	return !uf::matrix::equals( *this, matrix );
}

// 	Equality checking
// 	Equality check between two matrices (less than)
template<typename T> std::size_t uf::matrix::compareTo( const T& left, const T& right ) {
	return 1;
}
// 	Equality check between two matrices (equals)
template<typename T> bool uf::matrix::equals( const T& left, const T& right ) {
	return uf::matrix::compareTo(left, right) == 0;
}
// 	Basic arithmetic
// 	Multiplies two matrices of same type and size together
template<typename T> pod::Matrix<T,4,4> uf::matrix::multiply( const pod::Matrix<T,4,4>& left, const pod::Matrix<T,4,4>& right ) {
	pod::Matrix<T,4,4> res;
#if UF_USE_SIMD
	auto row1 = uf::simd::load(&left[0]);
	auto row2 = uf::simd::load(&left[4]);
	auto row3 = uf::simd::load(&left[8]);
	auto row4 = uf::simd::load(&left[12]);
	for(int i=0; i<4; i++) {
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
#elif 1
	// 
	float* dstPtr = &res[0];
	const float* leftPtr = &right[0];

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			const float* rightPtr = &left[0] + j;

			float sum = leftPtr[0] * rightPtr[0];
			for (size_t n = 1; n < 4; ++n) {
				rightPtr += 4;
				sum += leftPtr[n] * rightPtr[0];
			}
			*dstPtr++ = sum;
		}
		leftPtr += 4;
	}
	return res;
#elif 1
	// don't know if it's more performant than below
	std::size_t i = 0;
	for ( size_t col = 0; col < 4; col++ ) {
		for ( size_t row = 0; row < 4; row++ ) {
			res[i++] = uf::vector::dot( { right[0+col*4], right[1+col*4], right[2+col*4], right[3+col*4] }, { left[row+0*4], left[row+1*4], left[row+2*4], left[row+3*4] } );
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
	static const std::size_t R = T::rows;
	static const std::size_t C = T::columns;

	pod::Matrix<typename T::type_t,R,C> res;
#if 1
	float* dstPtr = &res[0];
	const float* leftPtr = &right[0];

	for (size_t i = 0; i < R; ++i) {
		for (size_t j = 0; j < C; ++j) {
			const float* rightPtr = &left[0] + j;

			float sum = leftPtr[0] * rightPtr[0];
			for (size_t n = 1; n < C; ++n) {
				rightPtr += C;
				sum += leftPtr[n] * rightPtr[0];
			}
			*dstPtr++ = sum;
		}
		leftPtr += C;
	}
#else
	std::size_t i = 0;
	for ( size_t col = 0; col < R; col++ ) {
		for ( size_t row = 0; row < C; row++ ) {
			auto& sum = res[i++];
			for ( size_t i = 0; i < C; i++ )
				sum += right[i + col * C] * left[row + i * R];
		}
	}
#endif
	return res;
}
// 	Transpose matrix
template<typename T> T uf::matrix::transpose( const T& matrix ) {
	static const std::size_t R = T::rows;
	static const std::size_t C = T::columns;
	T transpose;

	for ( typename T::type_t col = 0; col < C; ++col )
	for ( typename T::type_t row = 0; row < R; ++row )
		transpose[col * R + row] = matrix[row * C + col];

	return transpose;
}
// 	Flip sign of all components
template<typename T> T uf::matrix::inverse( const T& matrix ) {
	if ( T::rows != 4 ) return matrix;
	if ( T::columns != 4 ) return matrix;

	const typename T::type_t* m = &matrix[0];
	typename T::type_t inv[16], det;
	std::size_t i;

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
	T inverted;
	for (i = 0; i < 16; i++) inverted[i] = inv[i] * det;
	return inverted;
}
// 	Writes to first value
// 	Multiplies two matrices of same type and size together
template<typename T> pod::Matrix<typename T::type_t, T::columns, T::columns> uf::matrix::multiply( T& left, const T& right ) {
	return left = uf::matrix::multiply((const T&) left, right);
}
template<typename T> pod::Vector3t<T> uf::matrix::multiply( const pod::Matrix4t<T>& mat, const pod::Vector3t<T>& vector ) {
	return {
		vector[0] * mat[0] + vector[1] * mat[4] + vector[2] * mat[8] + vector[3] * mat[12],
		vector[0] * mat[1] + vector[1] * mat[5] + vector[2] * mat[9] + vector[3] * mat[13],
		vector[0] * mat[2] + vector[1] * mat[6] + vector[2] * mat[10] + vector[3] * mat[14]
	//	vector[0] * mat[3] + vector[1] * mat[7] + vector[2] * mat[11] + vector[3] * mat[15]
	};
}
template<typename T> pod::Vector4t<T> uf::matrix::multiply( const pod::Matrix4t<T>& mat, const pod::Vector4t<T>& vector ) {
	return {
		vector[0] * mat[0] + vector[1] * mat[4] + vector[2] * mat[8] + vector[3] * mat[12],
		vector[0] * mat[1] + vector[1] * mat[5] + vector[2] * mat[9] + vector[3] * mat[13],
		vector[0] * mat[2] + vector[1] * mat[6] + vector[2] * mat[10] + vector[3] * mat[14],
		vector[0] * mat[3] + vector[1] * mat[7] + vector[2] * mat[11] + vector[3] * mat[15]
	};
}
// 	Flip sign of all components
template<typename T> T& uf::matrix::invert( T& matrix ) {
	return matrix = uf::matrix::inverse((const T&) matrix);
}
// 	Complex arithmetic
template<typename T> T uf::matrix::translate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	T res = matrix;
	res[12] = vector.x;
	res[13] = vector.y;
	res[14] = vector.z;
	return res;
}
template<typename T> T uf::matrix::rotate( const T& matrix, const pod::Vector3t<typename T::type_t>& vector ) {
	T res = matrix;
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
	T res = matrix;
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
template<typename T> T& uf::matrix::copy( T& destination, const T& source ) {
	for ( std::size_t i = 0; i < 16; ++i )
		destination[i] = source[i];
	return destination;
}
template<typename T> T& uf::matrix::copy( T& destination, typename T::type_t* const source ) {
	for ( std::size_t i = 0; i < 16; ++i )
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
ext::json::Value /*UF_API*/ uf::matrix::encode( const pod::Matrix<T,R,C>& m ) {
	ext::json::Value json;
	for ( size_t i = 0; i < R*C; ++i ) json[i] = m[i];
	return json;
}
template<typename T, size_t R, size_t C>
pod::Matrix<T,R,C>& /*UF_API*/ uf::matrix::decode( const ext::json::Value& json, pod::Matrix<T,R,C>& m ) {
	for ( size_t i = 0; i < R*C; ++i ) m[i] = json[i].as<T>();
	return m;
}

template<typename T, size_t R, size_t C>
pod::Matrix<T,R,C> /*UF_API*/ uf::matrix::decode( const ext::json::Value& json, const pod::Matrix<T,R,C>& _m ) {
	pod::Matrix<T,R,C> m = _m;
	for ( size_t i = 0; i < R*C; ++i ) m[i] = json[i].as<T>();
	return m;
}

template<typename T, size_t R, size_t C>
std::string /*UF_API*/ uf::matrix::toString( const pod::Matrix<T,R,C>& m ) {
	std::stringstream ss;
	ss << "Matrix(\n\t";
	for ( size_t c = 0; c < C; ++c ) {
		for ( size_t r = 0; r < R; ++r ) {
			ss << m[r+c*C] << ", ";
		}
		if ( c + 1 < C ) ss << "\n\t";
	}
	ss << "\n)";
	return ss.str();
}