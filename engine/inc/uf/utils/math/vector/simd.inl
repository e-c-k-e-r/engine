#include <uf/utils/memory/alignment.h>

template<typename T>
inline uf::simd::value<T>::value() {}
template<typename T>
inline uf::simd::value<T>::value(const T* f) : m_value(uf::simd::load(f)) {}
template<typename T>
inline uf::simd::value<T>::value(T f) : m_value(uf::simd::set(f)) {}
template<typename T>
inline uf::simd::value<T>::value(T f0, T f1, T f2, T f3) : m_value(uf::simd::set(f0,f1,f2,f3)) {}
template<typename T>
inline uf::simd::value<T>::value(const value_type& rhs) : m_value(rhs) {}
template<typename T>
inline uf::simd::value<T>::value(const value& rhs) : m_value(rhs.m_value) {}

template<typename T>
inline uf::simd::value<T>::value(const pod::Vector<T,1>& rhs) : value((T) rhs[0]){}
template<typename T>
inline uf::simd::value<T>::value(const pod::Vector<T,2>& rhs) : value((T) rhs[0], (T) rhs[1], 0, 0){}
template<typename T>
inline uf::simd::value<T>::value(const pod::Vector<T,3>& rhs) : value((T) rhs[0], (T) rhs[1], (T) rhs[2], 0){}
template<typename T>
inline uf::simd::value<T>::value(const pod::Vector<T,4>& rhs) : value((T) rhs[0], (T) rhs[1], (T) rhs[2], (T) rhs[3]){}

template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator+( const value& rhs ) {
	return uf::simd::add( *this, rhs );
}
template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator-( const value& rhs ) {
	return uf::simd::sub( *this, rhs );
}
template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator*( const value& rhs ) {
	return uf::simd::mul( *this, rhs );
}
template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator/( const value& rhs ) {
	return uf::simd::div( *this, rhs );
}
template<typename T>
inline uf::simd::value<T>& uf::simd::value<T>::operator=(const uf::simd::value<T>::value_type& rhs) {
	m_value = rhs;
	return *this;
}
template<typename T>
inline uf::simd::value<T>& uf::simd::value<T>::operator=(const value& rhs) {
	m_value = rhs.m_value;
	return *this;
}
template<typename T>
inline uf::simd::value<T>& uf::simd::value<T>::operator=(const pod::Vector4f& rhs) {
	m_value = uf::simd::load(&rhs[0]);
	return *this;
}
template<typename T>
inline uf::simd::value<T>::operator uf::simd::value<T>::value_type() const {
	return m_value;
}

template<typename T>
template<size_t N>
inline uf::simd::value<T>::operator pod::Vector<T,N>() const {
	return uf::simd::vector<N>(*this);
}

template<size_t N>
inline pod::Vector<float,N> uf::simd::vector( const uf::simd::value<float> v ){
	pod::Vector4f r;
	uf::simd::store( v, &r[0] );
	return uf::vector::cast<float,N>(r);
}
template<size_t N>
inline pod::Vector<int,N> uf::simd::vector( const uf::simd::value<int> v ){
	pod::Vector4i r;
	uf::simd::store( v, &r[0] );
	return uf::vector::cast<int,N>(r);
}
template<size_t N>
inline pod::Vector<uint,N> uf::simd::vector( const uf::simd::value<uint> v ){
	pod::Vector4ui r;
	uf::simd::store( v, &r[0] );
	return uf::vector::cast<uint,N>(r);
}

inline uf::simd::value<float> /*UF_API*/ uf::simd::load( const float* f ) {
#if UF_VECTOR_ALIGNED
	return _mm_load_ps(f);
#else
	if ( uf::aligned(f, 16) ) return _mm_load_ps(f);

	alignas(16) float s[4];
	memcpy( &s[0], f, sizeof(float) * 4 );
	return _mm_loadu_ps(s);
#endif
}
inline void /*UF_API*/ uf::simd::store( uf::simd::value<float> v, float* f ) {
#if UF_VECTOR_ALIGNED
	return _mm_store_ps(f, v);
#else
	if ( uf::aligned(f, 16) ) return _mm_store_ps(f, v);

	alignas(16) float s[4];
	_mm_store_ps(&s[0], v);
	memcpy( f, &s[0], sizeof(float) * 4 );
#endif
}
inline uf::simd::value<float> /*UF_API*/ uf::simd::set( float f ) {
	return _mm_set1_ps(f);
}
inline uf::simd::value<float> /*UF_API*/ uf::simd::set( float x, float y, float z, float w ) {
	return _mm_setr_ps(x, y, z, w);
}
inline uf::simd::value<float> /*UF_API*/ uf::simd::add( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_add_ps( x, y );
}
inline uf::simd::value<float> /*UF_API*/ uf::simd::sub( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_sub_ps( x, y );
}
inline uf::simd::value<float> /*UF_API*/ uf::simd::mul( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_mul_ps( x, y );
}
inline uf::simd::value<float> /*UF_API*/ uf::simd::div( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_div_ps( x, y );
}
/*
inline uf::simd::value<float> uf::simd::hadd( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_hadd_ps( x, y );
}
*/
inline uf::simd::value<float> /*UF_API*/ uf::simd::min( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_min_ps( x, y );
}
inline uf::simd::value<float> /*UF_API*/ uf::simd::max( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_max_ps( x, y );
}
inline uf::simd::value<float> /*UF_API*/ uf::simd::sqrt( uf::simd::value<float> v ) {
	return _mm_sqrt_ps( v );
}
inline float /*UF_API*/ uf::simd::dot( uf::simd::value<float> x, uf::simd::value<float> y ) {
#if SSE_INSTR_SET >= 5
	float res;
	__m128 result = _mm_dp_ps(x, y, 0xFF);
	_mm_store_ss(&res, result);
	return res;
//	return uf::simd::vector<float,4>( result )[0];
#elif SSE_INSTR_SET >= 3
	__m128 mulRes = _mm_mul_ps(x, y);
	__m128 shufReg = _mm_movehdup_ps(mulRes);
	__m128 sumsReg = _mm_add_ps(mulRes, shufReg);
    shufReg = _mm_movehl_ps(shufReg, sumsReg);
    sumsReg = _mm_add_ss(sumsReg, shufReg);
    return _mm_cvtss_f32(sumsReg);
#else
	return uf::vector::sum( uf::simd::vector( uf::simd::mul( x, y ) ) );
#endif
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::load( const int32_t* f ) {
#if SSE_INSTR_SET >= 3
#if UF_VECTOR_ALIGNED
	return _mm_load_si128((__m128i*) f);
#else
	if ( uf::aligned(f, 16) ) return _mm_load_si128((__m128i*) f);

	alignas(16) int32_t s[4];
	memcpy( &s[0], f, sizeof(int32_t) * 4 );
	return _mm_load_si128((__m128i*) s);
#endif
#else
	return uf::simd::value<int32_t>( f[0], f[1], f[2], f[3] );
#endif
}
inline void /*UF_API*/ uf::simd::store( uf::simd::value<int32_t> v, int32_t* f ) {
#if SSE_INSTR_SET >= 3
#if UF_VECTOR_ALIGNED
	return _mm_store_si128((__m128i*) f, v);
#else
	if ( uf::aligned(f, 16) ) return _mm_store_si128((__m128i*) f, v);

	alignas(16) int32_t s[4];
	_mm_store_si128((__m128i*) &s[0], v);
	memcpy( f, &s[0], sizeof(int32_t) * 4 );
#endif
#else
	union {
		__m128i x;
		int32_t y[4];
	} kludge;
	kludge.x = v;
	f[0] = kludge.y[0];
	f[1] = kludge.y[1];
	f[2] = kludge.y[2];
	f[3] = kludge.y[3];
#endif
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::set( int32_t f ) {
	return _mm_set1_epi32(f);
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::set( int32_t x, int32_t y, int32_t z, int32_t w ) {
	return _mm_setr_epi32(x, y, z, w);
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::add( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] + Y[0], X[1] + Y[1], X[2] + Y[2], X[3] + Y[3] );
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::sub( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] - Y[0], X[1] - Y[1], X[2] - Y[2], X[3] - Y[3] );
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::mul( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] * Y[0], X[1] * Y[1], X[2] * Y[2], X[3] * Y[3] );
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::div( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] / Y[0], X[1] / Y[1], X[2] / Y[2], X[3] / Y[3] );
}
/*
inline uf::simd::value<int32_t> uf::simd::hadd( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] + Y[0], X[1] + Y[1], X[2] + Y[2], X[3] + Y[3] );
}
*/
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::min( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( std::min(X[0], Y[0]), std::min(X[1], Y[1]), std::min(X[2], Y[2]), std::min(X[3], Y[3]) );
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::max( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( std::max(X[0], Y[0]), std::max(X[1], Y[1]), std::max(X[2], Y[2]), std::max(X[3], Y[3]) );
}
inline uf::simd::value<int32_t> /*UF_API*/ uf::simd::sqrt( uf::simd::value<int32_t> v ) {
	auto V = uf::simd::vector( v );
	return uf::simd::set( (int32_t) std::sqrt(V[0]), (int32_t) std::sqrt(V[1]), (int32_t) std::sqrt(V[2]), (int32_t) std::sqrt(V[3]) );
}
inline int32_t /*UF_API*/ uf::simd::dot( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return X[0] * Y[0] + X[1] * Y[1] + X[2] * Y[2] + X[3] * Y[3];
}

inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::load( const uint32_t* f ) {
#if SSE_INSTR_SET >= 3
#if UF_VECTOR_ALIGNED
	return _mm_load_si128((__m128i*) f);
#else
	if ( uf::aligned(f, 16) ) return _mm_load_si128((__m128i*) f);

	alignas(16) uint32_t s[4];
	memcpy( &s[0], f, sizeof(uint32_t) * 4 );
	return _mm_load_si128((__m128i*) &s[0]);
#endif
#else
	return uf::simd::value<uint32_t>( f[0], f[1], f[2], f[3] );
#endif
}
inline void /*UF_API*/ uf::simd::store( uf::simd::value<uint32_t> v, uint32_t* f ) {
#if SSE_INSTR_SET >= 3
#if UF_VECTOR_ALIGNED
	return _mm_store_si128((__m128i*) f, v);
#else
	if ( uf::aligned(f, 16) ) return _mm_store_si128((__m128i*) f, v);

	alignas(16) uint32_t s[4];
	_mm_store_si128((__m128i*) &s[0], v);
	memcpy( f, &s[0], sizeof(uint32_t) * 4 );
#endif
#else
	union {
		__m128i x;
		uint32_t y[4];
	} kludge;
	kludge.x = v;
	f[0] = kludge.y[0];
	f[1] = kludge.y[1];
	f[2] = kludge.y[2];
	f[3] = kludge.y[3];
#endif
}
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::set( uint32_t f ) {
#if 0
	union {
		__m128i x;
		uint32_t y[4];
	} kludge;
	kludge.y[0] = f;
	kludge.y[1] = f;
	kludge.y[2] = f;
	kludge.y[3] = f;
#else
	return _mm_set1_epi32(f);
#endif
}
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::set( uint32_t x, uint32_t y, uint32_t z, uint32_t w ) {
#if 0
	union {
		__m128i x;
		uint32_t y[4];
	} kludge;
	kludge.y[0] = x;
	kludge.y[1] = y;
	kludge.y[2] = z;
	kludge.y[3] = w;
#else
	return _mm_setr_epi32(x, y, z, w);
#endif
}
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::add( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] + Y[0], X[1] + Y[1], X[2] + Y[2], X[3] + Y[3] );
}
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::sub( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] - Y[0], X[1] - Y[1], X[2] - Y[2], X[3] - Y[3] );
}
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::mul( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] * Y[0], X[1] * Y[1], X[2] * Y[2], X[3] * Y[3] );
}
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::div( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] / Y[0], X[1] / Y[1], X[2] / Y[2], X[3] / Y[3] );
}
/*
inline uf::simd::value<uint32_t> uf::simd::hadd( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( X[0] + Y[0], X[1] + Y[1], X[2] + Y[2], X[3] + Y[3] );
}
*/
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::min( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( std::min(X[0], Y[0]), std::min(X[1], Y[1]), std::min(X[2], Y[2]), std::min(X[3], Y[3]) );
}
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::max( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return uf::simd::set( std::max(X[0], Y[0]), std::max(X[1], Y[1]), std::max(X[2], Y[2]), std::max(X[3], Y[3]) );
}
inline uf::simd::value<uint32_t> /*UF_API*/ uf::simd::sqrt( uf::simd::value<uint32_t> v ) {
	auto V = uf::simd::vector( v );
	return uf::simd::set( (uint32_t) std::sqrt(V[0]), (uint32_t) std::sqrt(V[1]), (uint32_t) std::sqrt(V[2]), (uint32_t) std::sqrt(V[3]) );
}
inline uint32_t /*UF_API*/ uf::simd::dot( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return X[0] * Y[0] + X[1] * Y[1] + X[2] * Y[2] + X[3] * Y[3];
}