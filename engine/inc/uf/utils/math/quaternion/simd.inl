namespace uf {
	namespace simd {
		inline vector<float> /*UF_API*/ quatMul( vector<float>, vector<float> );
		inline vector<float> /*UF_API*/ quatRot_3f( vector<float>, vector<float> );
		inline pod::Matrix4f /*UF_API*/ quatMat( vector<float> );
	}
}

inline uf::simd::vector<float> uf::simd::quatMul( uf::simd::vector<float> Q1, uf::simd::vector<float> Q2 ) {
	// broadcast q1 components
	__m128 x1 = _mm_shuffle_ps(Q1, Q1, _MM_SHUFFLE(0,0,0,0));
	__m128 y1 = _mm_shuffle_ps(Q1, Q1, _MM_SHUFFLE(1,1,1,1));
	__m128 z1 = _mm_shuffle_ps(Q1, Q1, _MM_SHUFFLE(2,2,2,2));
	__m128 w1 = _mm_shuffle_ps(Q1, Q1, _MM_SHUFFLE(3,3,3,3));

	// broadcast q2 components
	__m128 x2 = _mm_shuffle_ps(Q2, Q2, _MM_SHUFFLE(0,0,0,0));
	__m128 y2 = _mm_shuffle_ps(Q2, Q2, _MM_SHUFFLE(1,1,1,1));
	__m128 z2 = _mm_shuffle_ps(Q2, Q2, _MM_SHUFFLE(2,2,2,2));
	__m128 w2 = _mm_shuffle_ps(Q2, Q2, _MM_SHUFFLE(3,3,3,3));

	// compute each component
	__m128 X = _mm_add_ps(
		_mm_add_ps(_mm_mul_ps(w1, x2), _mm_mul_ps(x1, w2)),
		_mm_sub_ps(_mm_mul_ps(y1, z2), _mm_mul_ps(z1, y2))
	);

	__m128 Y = _mm_add_ps(
		_mm_add_ps(_mm_mul_ps(w1, y2), _mm_mul_ps(y1, w2)),
		_mm_sub_ps(_mm_mul_ps(z1, x2), _mm_mul_ps(x1, z2))
	);

	__m128 Z = _mm_add_ps(
		_mm_add_ps(_mm_mul_ps(w1, z2), _mm_mul_ps(z1, w2)),
		_mm_sub_ps(_mm_mul_ps(x1, y2), _mm_mul_ps(y1, x2))
	);

	__m128 W = _mm_sub_ps(
		_mm_mul_ps(w1, w2),
		_mm_add_ps(
			_mm_add_ps(_mm_mul_ps(x1, x2), _mm_mul_ps(y1, y2)),
			_mm_mul_ps(z1, z2)
		)
	);

	// pack back into (x,y,z,w)
	__m128 result = _mm_movelh_ps(_mm_unpacklo_ps(X, Y), _mm_unpacklo_ps(Z, W));
	return result;
}
inline uf::simd::vector<float> uf::simd::quatRot_3f( uf::simd::vector<float> Q, uf::simd::vector<float> V ) {
	// extract q.xyz and q.w
	__m128 qxyz = _mm_and_ps(Q, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1))); // mask out w
	__m128 qw   = _mm_shuffle_ps(Q, Q, _MM_SHUFFLE(3,3,3,3));

	// cross(q.xyz, v)
	__m128 q_yzx = _mm_shuffle_ps(qxyz, qxyz, _MM_SHUFFLE(3,0,2,1));
	__m128 v_yzx = _mm_shuffle_ps(V, V, _MM_SHUFFLE(3,0,2,1));
	__m128 cross1 = _mm_sub_ps(_mm_mul_ps(qxyz, v_yzx), _mm_mul_ps(q_yzx, V));
	cross1 = _mm_shuffle_ps(cross1, cross1, _MM_SHUFFLE(3,0,2,1));

	// 2 * w * cross(q,v)
	__m128 term1 = _mm_mul_ps(_mm_mul_ps(cross1, qw), _mm_set1_ps(2.0f));

	// cross(q, cross(q,v))
	__m128 c1_yzx = _mm_shuffle_ps(cross1, cross1, _MM_SHUFFLE(3,0,2,1));
	__m128 cross2 = _mm_sub_ps(_mm_mul_ps(qxyz, c1_yzx), _mm_mul_ps(q_yzx, cross1));
	cross2 = _mm_shuffle_ps(cross2, cross2, _MM_SHUFFLE(3,0,2,1));

	// 2 * cross(q, cross(q,v))
	__m128 term2 = _mm_mul_ps(cross2, _mm_set1_ps(2.0f));

	// v + term1 + term2
	__m128 result = _mm_add_ps(_mm_add_ps(V, term1), term2);

	return result;
}

inline pod::Matrix4f uf::simd::quatMat( uf::simd::vector<float> Q ) {
	// Shuffle out components
	__m128 qx = _mm_shuffle_ps(Q, Q, _MM_SHUFFLE(0,0,0,0));
	__m128 qy = _mm_shuffle_ps(Q, Q, _MM_SHUFFLE(1,1,1,1));
	__m128 qz = _mm_shuffle_ps(Q, Q, _MM_SHUFFLE(2,2,2,2));
	__m128 qw = _mm_shuffle_ps(Q, Q, _MM_SHUFFLE(3,3,3,3));

	// Compute squares
	__m128 xx = _mm_mul_ps(qx, qx);
	__m128 yy = _mm_mul_ps(qy, qy);
	__m128 zz = _mm_mul_ps(qz, qz);

	// Cross terms
	__m128 xy = _mm_mul_ps(qx, qy);
	__m128 xz = _mm_mul_ps(qx, qz);
	__m128 yz = _mm_mul_ps(qy, qz);
	__m128 xw = _mm_mul_ps(qx, qw);
	__m128 yw = _mm_mul_ps(qy, qw);
	__m128 zw = _mm_mul_ps(qz, qw);

	__m128 two = _mm_set1_ps(2.0f);

	xx = _mm_mul_ps(xx, two);
	yy = _mm_mul_ps(yy, two);
	zz = _mm_mul_ps(zz, two);
	xy = _mm_mul_ps(xy, two);
	xz = _mm_mul_ps(xz, two);
	yz = _mm_mul_ps(yz, two);
	xw = _mm_mul_ps(xw, two);
	yw = _mm_mul_ps(yw, two);
	zw = _mm_mul_ps(zw, two);

	pod::Matrix4f M;

	M[0] = 1.0f - _mm_cvtss_f32(yy) - _mm_cvtss_f32(zz);
	M[1] = _mm_cvtss_f32(xy) + _mm_cvtss_f32(zw);
	M[2] = _mm_cvtss_f32(xz) - _mm_cvtss_f32(yw);
	M[3] = 0.0f;

	M[4] = _mm_cvtss_f32(xy) - _mm_cvtss_f32(zw);
	M[5] = 1.0f - _mm_cvtss_f32(xx) - _mm_cvtss_f32(zz);
	M[6] = _mm_cvtss_f32(yz) + _mm_cvtss_f32(xw);
	M[7] = 0.0f;

	M[8]  = _mm_cvtss_f32(xz) + _mm_cvtss_f32(yw);
	M[9]  = _mm_cvtss_f32(yz) - _mm_cvtss_f32(xw);
	M[10] = 1.0f - _mm_cvtss_f32(xx) - _mm_cvtss_f32(yy);
	M[11] = 0.0f;

	M[12] = 0.0f;
	M[13] = 0.0f;
	M[14] = 0.0f;
	M[15] = 1.0f;

	return M;
}