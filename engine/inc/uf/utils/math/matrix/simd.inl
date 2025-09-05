namespace uf {
	namespace simd {
		template<typename T>
		class alignas(16) matrix_value {
		public:
			typedef typename traits<T>::value value_type;
			value_type m[4]; // 4 x 4

			inline matrix_value();
			inline matrix_value(const pod::Matrix<T,4>& rhs);

			inline bool operator==(const matrix_value&) const;
			inline operator pod::Matrix<T,4>() const;
		};
	}

	namespace simd {
		inline uf::simd::matrix_value<float> matMult( const uf::simd::matrix_value<float>& A, const uf::simd::matrix_value<float>& B );
		inline uf::simd::vector<float> matMult( const uf::simd::matrix_value<float>& A, uf::simd::vector<float> B );
		inline uf::simd::matrix_value<float> matTranspose( const uf::simd::matrix_value<float>& M );
		inline bool matEquals( const uf::simd::matrix_value<float>& A, const uf::simd::matrix_value<float>& B, float eps );
	}
}

namespace {
	__attribute__((target("default")))
	uf::simd::matrix_value<float> matMult_impl(const uf::simd::matrix_value<float>& A, const uf::simd::matrix_value<float>& B) {
		uf::simd::matrix_value<float> R;
		uf::simd::matrix_value<float> Bt = uf::simd::matTranspose(B);
		FOR_EACH(4, {
			__m128 bcol = B.m[i];

			__m128 vx = _mm_shuffle_ps(bcol, bcol, 0x00); // xxxx
			__m128 vy = _mm_shuffle_ps(bcol, bcol, 0x55); // yyyy
			__m128 vz = _mm_shuffle_ps(bcol, bcol, 0xAA); // zzzz
			__m128 vw = _mm_shuffle_ps(bcol, bcol, 0xFF); // wwww

			R.m[i] = _mm_add_ps(
				_mm_add_ps(
					_mm_mul_ps(A.m[0], vx),
					_mm_mul_ps(A.m[1], vy)),
				_mm_add_ps(
					_mm_mul_ps(A.m[2], vz),
					_mm_mul_ps(A.m[3], vw))
			);
		});

		return R;

	}
	#if 1
	__attribute__((target("sse4.1")))
	uf::simd::matrix_value<float> matMult_impl(const uf::simd::matrix_value<float>& A, const uf::simd::matrix_value<float>& B) {
		uf::simd::matrix_value<float> R;
		uf::simd::matrix_value<float> Bt = uf::simd::matTranspose(B);

		FOR_EACH(4, {
			__m128 bcol = B.m[i];

			__m128 vx = _mm_shuffle_ps(bcol, bcol, 0x00); // xxxx
			__m128 vy = _mm_shuffle_ps(bcol, bcol, 0x55); // yyyy
			__m128 vz = _mm_shuffle_ps(bcol, bcol, 0xAA); // zzzz
			__m128 vw = _mm_shuffle_ps(bcol, bcol, 0xFF); // wwww

			R.m[i] = _mm_add_ps(
				_mm_add_ps(
					_mm_mul_ps(A.m[0], vx),
					_mm_mul_ps(A.m[1], vy)),
				_mm_add_ps(
					_mm_mul_ps(A.m[2], vz),
					_mm_mul_ps(A.m[3], vw))
			);
		});

		return R;
	}
	#endif
	#if 1
	__attribute__((target("avx2,fma")))
	uf::simd::matrix_value<float> matMult_impl(const uf::simd::matrix_value<float>& A, const uf::simd::matrix_value<float>& B) {
		uf::simd::matrix_value<float> R;
		uf::simd::matrix_value<float> Bt = uf::simd::matTranspose(B);

		FOR_EACH(4, {
			__m128 bcol = B.m[i];

			__m256 vx = _mm256_broadcastss_ps(bcol);						  		// xxxx
			__m256 vy = _mm256_broadcastss_ps(_mm_shuffle_ps(bcol, bcol, 0x55)); 	// yyyy
			__m256 vz = _mm256_broadcastss_ps(_mm_shuffle_ps(bcol, bcol, 0xAA)); 	// zzzz
			__m256 vw = _mm256_broadcastss_ps(_mm_shuffle_ps(bcol, bcol, 0xFF)); 	// wwww

			__m256 a0 = _mm256_castps128_ps256(A.m[0]);
			__m256 a1 = _mm256_castps128_ps256(A.m[1]);
			__m256 a2 = _mm256_castps128_ps256(A.m[2]);
			__m256 a3 = _mm256_castps128_ps256(A.m[3]);

			__m256 r =	_mm256_fmadd_ps(a0, vx,
						_mm256_fmadd_ps(a1, vy,
						_mm256_fmadd_ps(a2, vz,
						_mm256_mul_ps(a3, vw))));

			__m128 r128 = _mm_add_ps(
				_mm256_castps256_ps128(r),
				_mm256_extractf128_ps(r, 1)
			);

			R.m[i] = r128;
		});

		return R;
	}
	#endif
	#if 1
	__attribute__((target("avx512f")))
	uf::simd::matrix_value<float> matMult_impl( const uf::simd::matrix_value<float>& A, const uf::simd::matrix_value<float>& B) {
		uf::simd::matrix_value<float> R;
		uf::simd::matrix_value<float> Bt = uf::simd::matTranspose(B);

		FOR_EACH(4, {
			__m128 bcol = B.m[i];

			__m512 vx = _mm512_set1_ps(((const float*)&bcol)[0]); // xxxx
			__m512 vy = _mm512_set1_ps(((const float*)&bcol)[1]); // yyyy
			__m512 vz = _mm512_set1_ps(((const float*)&bcol)[2]); // zzzz
			__m512 vw = _mm512_set1_ps(((const float*)&bcol)[3]); // wwww

			__m512 a0 = _mm512_castps128_ps512(A.m[0]);
			__m512 a1 = _mm512_castps128_ps512(A.m[1]);
			__m512 a2 = _mm512_castps128_ps512(A.m[2]);
			__m512 a3 = _mm512_castps128_ps512(A.m[3]);

			__m512 r = 	_mm512_fmadd_ps(a0, vx,
						_mm512_fmadd_ps(a1, vy,
						_mm512_fmadd_ps(a2, vz,
						_mm512_mul_ps(a3, vw))));

			__m128 r128 = _mm_add_ps(
				_mm_add_ps(
					_mm512_castps512_ps128(r),		  // low 128
					_mm512_extractf32x4_ps(r, 1)),	  // next 128
				_mm_add_ps(
					_mm512_extractf32x4_ps(r, 2),	   // next 128
					_mm512_extractf32x4_ps(r, 3))	   // high 128
			);

			R.m[i] = r128;
		});

		return R;
	}
	#endif

	__attribute__((target("default")))
	uf::simd::vector<float> matMult_impl( const uf::simd::matrix_value<float>& M, uf::simd::vector<float> v ) {
		__m128 vx = _mm_shuffle_ps(v, v, 0x00);
		__m128 vy = _mm_shuffle_ps(v, v, 0x55);
		__m128 vz = _mm_shuffle_ps(v, v, 0xAA);
		__m128 vw = _mm_shuffle_ps(v, v, 0xFF);

		__m128 r0 = _mm_mul_ps(M.m[0], vx);
		__m128 r1 = _mm_mul_ps(M.m[1], vy);
		__m128 r2 = _mm_mul_ps(M.m[2], vz);
		__m128 r3 = _mm_mul_ps(M.m[3], vw);

		return _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));
	}
	#if 1
	__attribute__((target("fma")))
	uf::simd::vector<float> matMult_impl( const uf::simd::matrix_value<float>& M, uf::simd::vector<float> v ) {
		__m128 vx = _mm_shuffle_ps(v, v, 0x00);
		__m128 vy = _mm_shuffle_ps(v, v, 0x55);
		__m128 vz = _mm_shuffle_ps(v, v, 0xAA);
		__m128 vw = _mm_shuffle_ps(v, v, 0xFF);

		return 	_mm_fmadd_ps(M.m[0], vx,
				_mm_fmadd_ps(M.m[1], vy,
				_mm_fmadd_ps(M.m[2], vz,
				_mm_mul_ps(M.m[3], vw))));
	}
	#endif
}

template<typename T>
inline uf::simd::matrix_value<T>::matrix_value() {}
template<typename T>
inline uf::simd::matrix_value<T>::matrix_value( const pod::Matrix<T,4>& mat ) {
	m[0] = _mm_loadu_ps(&mat[0]);
	m[1] = _mm_loadu_ps(&mat[4]);
	m[2] = _mm_loadu_ps(&mat[8]);
	m[3] = _mm_loadu_ps(&mat[12]);
}
template<typename T>
inline bool uf::simd::matrix_value<T>::operator==(const matrix_value& rhs) const {
	return uf::simd::matEquals( *this, rhs );
}
template<typename T>
inline uf::simd::matrix_value<T>::operator pod::Matrix<T,4>() const {
	pod::Matrix4f mat;
	_mm_storeu_ps(&mat[0],  m[0]);
	_mm_storeu_ps(&mat[4],  m[1]);
	_mm_storeu_ps(&mat[8],  m[2]);
	_mm_storeu_ps(&mat[12], m[3]);
	return mat;
}

inline uf::simd::matrix_value<float> uf::simd::matMult( const uf::simd::matrix_value<float>& A, const uf::simd::matrix_value<float>& B ) {
	return ::matMult_impl( A, B );
}
inline uf::simd::vector<float> uf::simd::matMult( const uf::simd::matrix_value<float>& M, uf::simd::vector<float> vec ) {
	return ::matMult_impl( M, vec );
}
inline uf::simd::matrix_value<float> uf::simd::matTranspose( const uf::simd::matrix_value<float>& M ) {
	uf::simd::matrix_value<float> R = M;
	_MM_TRANSPOSE4_PS(R.m[0], R.m[1], R.m[2], R.m[3]);
	return R;
}
inline bool uf::simd::matEquals( const uf::simd::matrix_value<float>& A, const uf::simd::matrix_value<float>& B, float eps ) {
	bool result = true;
	__m128 e = _mm_set1_ps(eps);
	FOR_EACH(4, {
		__m128 diff = _mm_sub_ps(A.m[i], B.m[i]);
		__m128 mask = _mm_cmpgt_ps(_mm_andnot_ps(_mm_set1_ps(-0.0f), diff), e);
		if (_mm_movemask_ps(mask)) result = false;
	});
	return result;
}