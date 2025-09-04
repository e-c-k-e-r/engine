namespace uf {
	namespace simd {
		inline value<float> /*UF_API*/ quatMul( value<float>, value<float> );
		inline value<float> /*UF_API*/ quatRot( value<float>, value<float> );
		inline pod::Matrix4f /*UF_API*/ quatMat( value<float> );
	}
}

inline uf::simd::value<float> uf::simd::quatMul( uf::simd::value<float> Q1, uf::simd::value<float> Q2 ) {
	//__m128 Q1 = q1;
	//__m128 Q2 = q2;

	// Broadcast q1.w, q1.x, q1.y, q1.z
	__m128 q1w = _mm_shuffle_ps(Q1, Q1, _MM_SHUFFLE(3,3,3,3));
	__m128 q1x = _mm_shuffle_ps(Q1, Q1, _MM_SHUFFLE(0,0,0,0));
	__m128 q1y = _mm_shuffle_ps(Q1, Q1, _MM_SHUFFLE(1,1,1,1));
	__m128 q1z = _mm_shuffle_ps(Q1, Q1, _MM_SHUFFLE(2,2,2,2));

	// Shuffle q2 into (x,y,z,w) permutations
	__m128 q2xyzw = Q2; // (x,y,z,w)
	__m128 q2wzyx = _mm_shuffle_ps(Q2, Q2, _MM_SHUFFLE(0,1,2,3)); // (w,z,y,x)
	__m128 q2yzxw = _mm_shuffle_ps(Q2, Q2, _MM_SHUFFLE(3,0,2,1)); // (y,z,x,w)
	__m128 q2zxyw = _mm_shuffle_ps(Q2, Q2, _MM_SHUFFLE(3,1,0,2)); // (z,x,y,w)

	// Compute terms
	__m128 t0 = _mm_mul_ps(q1w, q2xyzw); // w1 * (x2,y2,z2,w2)
	__m128 t1 = _mm_mul_ps(q1x, q2wzyx); // x1 * (w2,z2,y2,x2)
	__m128 t2 = _mm_mul_ps(q1y, q2yzxw); // y1 * (y2,z2,x2,w2)
	__m128 t3 = _mm_mul_ps(q1z, q2zxyw); // z1 * (z2,x2,y2,w2)

	// Signs: (+,+,+,+), (+,-,+,-), (-,+,-,+), (+,-,-,+)
	const __m128 sign1 = _mm_set_ps( 1.f,-1.f, 1.f,-1.f);
	const __m128 sign2 = _mm_set_ps(-1.f, 1.f,-1.f, 1.f);
	const __m128 sign3 = _mm_set_ps( 1.f,-1.f,-1.f, 1.f);

	t1 = _mm_mul_ps(t1, sign1);
	t2 = _mm_mul_ps(t2, sign2);
	t3 = _mm_mul_ps(t3, sign3);

	__m128 result = _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3));
	return result;
}
inline uf::simd::value<float> uf::simd::quatRot( uf::simd::value<float> Q, uf::simd::value<float> V ) {
	//__m128 Q = q; // (x,y,z,w)
	//__m128 V = v; // (vx,vy,vz,0)

	// Extract q.xyz and q.w
	__m128 qxyz = _mm_and_ps(Q, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1))); // mask out w
	__m128 qw   = _mm_shuffle_ps(Q, Q, _MM_SHUFFLE(3,3,3,3));

	// dot(q.xyz, v)
#if SSE_INSTR_SET >= 4
	__m128 dot_qv = _mm_dp_ps(qxyz, V, 0x71); // result in lowest lane
#else
	__m128 mul = _mm_mul_ps(qxyz, V);
	__m128 shuf = _mm_movehdup_ps(mul);
	__m128 sums = _mm_add_ps(mul, shuf);
	shuf = _mm_movehl_ps(shuf, sums);
	sums = _mm_add_ss(sums, shuf);
	__m128 dot_qv = sums;
#endif
	__m128 term1 = _mm_mul_ps(_mm_mul_ps(dot_qv, _mm_set1_ps(2.0f)), qxyz);

	// dot(q.xyz, q.xyz)
#if SSE_INSTR_SET >= 4
	__m128 dot_qq = _mm_dp_ps(qxyz, qxyz, 0x71);
#else
	__m128 mul2 = _mm_mul_ps(qxyz, qxyz);
	__m128 shuf2 = _mm_movehdup_ps(mul2);
	__m128 sums2 = _mm_add_ps(mul2, shuf2);
	shuf2 = _mm_movehl_ps(shuf2, sums2);
	sums2 = _mm_add_ss(sums2, shuf2);
	__m128 dot_qq = sums2;
#endif
	__m128 w2 = _mm_mul_ps(qw, qw);
	__m128 coeff = _mm_sub_ps(w2, dot_qq);
	__m128 term2 = _mm_mul_ps(coeff, V);

	// cross(q.xyz, v)
	__m128 q_yzx = _mm_shuffle_ps(qxyz, qxyz, _MM_SHUFFLE(3,0,2,1));
	__m128 v_yzx = _mm_shuffle_ps(V, V, _MM_SHUFFLE(3,0,2,1));
	__m128 cross = _mm_sub_ps(_mm_mul_ps(qxyz, v_yzx), _mm_mul_ps(q_yzx, V));
	cross = _mm_shuffle_ps(cross, cross, _MM_SHUFFLE(3,0,2,1));
	__m128 term3 = _mm_mul_ps(_mm_mul_ps(cross, qw), _mm_set1_ps(2.0f));

	// Final result
	__m128 result = _mm_add_ps(_mm_add_ps(term1, term2), term3);

	return result;
}
inline pod::Matrix4f uf::simd::quatMat( uf::simd::value<float> Q ) {
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