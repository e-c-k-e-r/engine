#include <uf/utils/memory/alignment.h>

namespace {
	inline __m128i bias_unsigned(__m128i v) {
		const __m128i signbit = _mm_set1_epi32(0x80000000);
		return _mm_xor_si128(v, signbit);
	}
}

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
inline uf::simd::value<T> uf::simd::value<T>::operator<( const value& rhs ) {
	return uf::simd::less( *this, rhs );
}
template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator<=( const value& rhs ) {
	return uf::simd::lessEquals( *this, rhs );
}
template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator>( const value& rhs ) {
	return uf::simd::greater( *this, rhs );
}
template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator>=( const value& rhs ) {
	return uf::simd::greaterEquals( *this, rhs );
}
template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator==( const value& rhs ) {
	return uf::simd::equals( *this, rhs );
}
template<typename T>
inline uf::simd::value<T> uf::simd::value<T>::operator!=( const value& rhs ) {
	return uf::simd::notEquals( *this, rhs );
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
inline uf::simd::value<T>& uf::simd::value<T>::operator=(const pod::Vector<T,4>& rhs) {
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
inline pod::Vector<int32_t,N> uf::simd::vector( const uf::simd::value<int32_t> v ){
	pod::Vector4i r;
	uf::simd::store( v, &r[0] );
	return uf::vector::cast<int32_t,N>(r);
}
template<size_t N>
inline pod::Vector<uint32_t,N> uf::simd::vector( const uf::simd::value<uint32_t> v ){
	pod::Vector4ui r;
	uf::simd::store( v, &r[0] );
	return uf::vector::cast<uint32_t,N>(r);
}

inline uf::simd::value<float> uf::simd::load( const float* f ) {
	// if ( uf::aligned(f, 16) ) return _mm_load_ps(f);
	return _mm_loadu_ps(f);
}
inline void uf::simd::store( uf::simd::value<float> v, float* f ) {
	/* if ( uf::aligned(f, 16) ) _mm_store_ps(f, v);
	else */ _mm_storeu_ps(f, v);
}
inline uf::simd::value<float> uf::simd::set( float f ) {
	return _mm_set1_ps(f);
}
inline uf::simd::value<float> uf::simd::set( float x, float y, float z, float w ) {
	return _mm_setr_ps(x, y, z, w);
}
inline uf::simd::value<float> uf::simd::add( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_add_ps( x, y );
}
inline uf::simd::value<float> uf::simd::sub( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_sub_ps( x, y );
}
inline uf::simd::value<float> uf::simd::mul( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_mul_ps( x, y );
}
inline uf::simd::value<float> uf::simd::div( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_div_ps( x, y );
}
/*
inline uf::simd::value<float> uf::simd::hadd( uf::simd::value<float> x, uf::simd::value<float> y ) {
#if 0
	return _mm_hadd_ps( x, y );
#else
	__m128 shuf = _mm_movehdup_ps(v);
	__m128 sums = _mm_add_ps(v, shuf);
	shuf = _mm_movehl_ps(shuf, sums);
	sums = _mm_add_ss(sums, shuf);
	return _mm_cvtss_f32(sums);
#endif
}
*/

inline uf::simd::value<float> uf::simd::min( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_min_ps( x, y );
}
inline uf::simd::value<float> uf::simd::max( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_max_ps( x, y );
}
inline bool uf::simd::all( uf::simd::value<float> mask) {
	return _mm_movemask_ps(mask) == 0xF; // all 4 bits set
}
inline bool uf::simd::any( uf::simd::value<float> mask) {
	return _mm_movemask_ps(mask) != 0x0; // any bit set
}
inline uf::simd::value<float> uf::simd::less( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_cmplt_ps( x, y );
}
inline uf::simd::value<float> uf::simd::lessEquals( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_cmple_ps( x, y );
}
inline uf::simd::value<float> uf::simd::greater( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_cmpgt_ps( x, y );
}
inline uf::simd::value<float> uf::simd::greaterEquals( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_cmpge_ps( x, y );
}
inline uf::simd::value<float> uf::simd::equals( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_cmpeq_ps( x, y );
}
inline uf::simd::value<float> uf::simd::notEquals( uf::simd::value<float> x, uf::simd::value<float> y ) {
	return _mm_cmpneq_ps( x, y );
}
inline uf::simd::value<float> uf::simd::sqrt( uf::simd::value<float> v ) {
	return _mm_sqrt_ps( v );
}
inline float uf::simd::dot( uf::simd::value<float> x, uf::simd::value<float> y ) {
#if SSE_INSTR_SET >= 5
	__m128 result = _mm_dp_ps(x, y, 0xF1);
	return _mm_cvtss_f32(result);
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
inline uf::simd::value<int32_t> uf::simd::load( const int32_t* f ) {
#if SSE_INSTR_SET >= 3
	// if ( uf::aligned(f, 16) ) return _mm_load_si128(reinterpret_cast<const __m128i*>(f));
	return _mm_loadu_si128(reinterpret_cast<const __m128i*>(f));
#else
	return uf::simd::value<int32_t>( f[0], f[1], f[2], f[3] );
#endif
}
inline void uf::simd::store( uf::simd::value<int32_t> v, int32_t* f ) {
#if SSE_INSTR_SET >= 3
	/*if ( uf::aligned(f, 16) ) _mm_store_si128(reinterpret_cast<__m128i*>(f), v);
	else*/ _mm_storeu_si128(reinterpret_cast<__m128i*>(f), v);
#else
	union { __m128i x; int32_t y[4]; } kludge;
	kludge.x = v;
	f[0] = kludge.y[0];
	f[1] = kludge.y[1];
	f[2] = kludge.y[2];
	f[3] = kludge.y[3];
#endif
}
inline uf::simd::value<int32_t> uf::simd::set( int32_t f ) {
	return _mm_set1_epi32(f);
}
inline uf::simd::value<int32_t> uf::simd::set( int32_t x, int32_t y, int32_t z, int32_t w ) {
	return _mm_setr_epi32(x, y, z, w);
}
inline uf::simd::value<int32_t> uf::simd::add( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	return _mm_add_epi32(x, y);
}
inline uf::simd::value<int32_t> uf::simd::sub( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	return _mm_sub_epi32(x, y);
}
inline uf::simd::value<int32_t> uf::simd::mul( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_mullo_epi32(x, y);
#else
	auto X = uf::simd::vector(x);
	auto Y = uf::simd::vector(y);
	return uf::simd::set(X[0]*Y[0], X[1]*Y[1], X[2]*Y[2], X[3]*Y[3]);
#endif
}
inline uf::simd::value<int32_t> uf::simd::div( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
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
inline uf::simd::value<int32_t> uf::simd::min( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_min_epi32(x, y);
#else
	auto X = uf::simd::vector(x);
	auto Y = uf::simd::vector(y);
	return uf::simd::set(std::min(X[0],Y[0]), std::min(X[1],Y[1]), std::min(X[2],Y[2]), std::min(X[3],Y[3]));
#endif
}
inline uf::simd::value<int32_t> uf::simd::max( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_max_epi32(x, y);
#else
	auto X = uf::simd::vector(x);
	auto Y = uf::simd::vector(y);
	return uf::simd::set(std::max(X[0],Y[0]), std::max(X[1],Y[1]), std::max(X[2],Y[2]), std::max(X[3],Y[3]));
#endif
}
inline bool uf::simd::all( uf::simd::value<int32_t> mask) {
	return _mm_movemask_epi8( mask ) == 0xFFFF; // all 4 bits set
}
inline bool uf::simd::any( uf::simd::value<int32_t> mask) {
	return _mm_movemask_epi8( mask ) != 0x0; // any bit set
}
inline uf::simd::value<int32_t> uf::simd::less( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_cmplt_epi32( x, y );
#else
	auto X = vector( x ), Y = vector( y );
	return set(X[0] < Y[0], X[1] < Y[1], X[2] < Y[2], X[3] < Y[3]);
#endif
}
inline uf::simd::value<int32_t> uf::simd::lessEquals( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
#if SSE_INSTR_SET >= 4
	__m128i gt = _mm_cmpgt_epi32(x, y);
	return _mm_xor_si128(gt, _mm_set1_epi32(-1));
#else
	auto X = vector( x ), Y = vector( y );
	return uf::simd::set(X[0] <= Y[0], X[1] <= Y[1], X[2] <= Y[2], X[3] <= Y[3]);
#endif
}
inline uf::simd::value<int32_t> uf::simd::greater( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_cmpgt_epi32( x, y );
#else
	auto X = vector( x ), Y = vector( y );
	return uf::simd::set(X[0] > Y[0], X[1] > Y[1], X[2] > Y[2], X[3] > Y[3]);
#endif
}
inline uf::simd::value<int32_t> uf::simd::greaterEquals( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
#if SSE_INSTR_SET >= 4
	__m128i gt = _mm_cmplt_epi32(x, y);
	return _mm_xor_si128(gt, _mm_set1_epi32(-1));
#else
	auto X = vector( x ), Y = vector( y );
	return uf::simd::set(X[0] >= Y[0], X[1] >= Y[1], X[2] >= Y[2], X[3] >= Y[3]);
#endif
}
inline uf::simd::value<int32_t> uf::simd::equals( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	return _mm_cmpeq_epi32(x, y);
}
inline uf::simd::value<int32_t> uf::simd::notEquals( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	return _mm_xor_si128(_mm_cmpeq_epi32(x, y), _mm_set1_epi32(-1));
}
inline uf::simd::value<int32_t> uf::simd::sqrt( uf::simd::value<int32_t> v ) {
	auto V = uf::simd::vector( v );
	return uf::simd::set( (int32_t) std::sqrt(V[0]), (int32_t) std::sqrt(V[1]), (int32_t) std::sqrt(V[2]), (int32_t) std::sqrt(V[3]) );
}
inline int32_t uf::simd::dot( uf::simd::value<int32_t> x, uf::simd::value<int32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return X[0] * Y[0] + X[1] * Y[1] + X[2] * Y[2] + X[3] * Y[3];
}

inline uf::simd::value<uint32_t> uf::simd::load( const uint32_t* f ) {
#if SSE_INSTR_SET >= 3
	// if ( uf::aligned(f, 16) ) return _mm_load_si128(reinterpret_cast<const __m128i*>(f));
	return _mm_loadu_si128(reinterpret_cast<const __m128i*>(f));
#else
	return uf::simd::value<uint32_t>( f[0], f[1], f[2], f[3] );
#endif
}
inline void uf::simd::store( uf::simd::value<uint32_t> v, uint32_t* f ) {
#if SSE_INSTR_SET >= 3
	/*if ( uf::aligned(f, 16) ) _mm_store_si128(reinterpret_cast<__m128i*>(f), v);
	else*/ _mm_storeu_si128(reinterpret_cast<__m128i*>(f), v);
#else
	union { __m128i x; uint32_t y[4]; } kludge;
	kludge.x = v;
	f[0] = kludge.y[0];
	f[1] = kludge.y[1];
	f[2] = kludge.y[2];
	f[3] = kludge.y[3];
#endif
}
inline uf::simd::value<uint32_t> uf::simd::set( uint32_t f ) {
	return _mm_set1_epi32(f);
}
inline uf::simd::value<uint32_t> uf::simd::set( uint32_t x, uint32_t y, uint32_t z, uint32_t w ) {
	return _mm_setr_epi32(x, y, z, w);
}
inline uf::simd::value<uint32_t> uf::simd::add( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	return _mm_add_epi32(x, y);
}
inline uf::simd::value<uint32_t> uf::simd::sub( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	return _mm_sub_epi32(x, y);
}
inline uf::simd::value<uint32_t> uf::simd::mul( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_mullo_epi32(x, y);
#else
	auto X = uf::simd::vector(x);
	auto Y = uf::simd::vector(y);
	return uf::simd::set(X[0]*Y[0], X[1]*Y[1], X[2]*Y[2], X[3]*Y[3]);
#endif
}
inline uf::simd::value<uint32_t> uf::simd::div( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
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
inline uf::simd::value<uint32_t> uf::simd::min( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_min_epu32(x, y); // unsigned min
#else
	auto X = uf::simd::vector(x);
	auto Y = uf::simd::vector(y);
	return uf::simd::set(std::min(X[0],Y[0]), std::min(X[1],Y[1]), std::min(X[2],Y[2]), std::min(X[3],Y[3]));
#endif
}
inline uf::simd::value<uint32_t> uf::simd::max( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_max_epu32(x, y); // unsigned max
#else
	auto X = uf::simd::vector(x);
	auto Y = uf::simd::vector(y);
	return uf::simd::set(std::max(X[0],Y[0]), std::max(X[1],Y[1]), std::max(X[2],Y[2]), std::max(X[3],Y[3]));
#endif
}
inline bool uf::simd::all( uf::simd::value<uint32_t> mask) {
	return _mm_movemask_epi8( mask ) == 0xFFFF; // all 4 bits set
}
inline bool uf::simd::any( uf::simd::value<uint32_t> mask) {
	return _mm_movemask_epi8( mask ) != 0x0; // any bit set
}
inline uf::simd::value<uint32_t> uf::simd::less( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_cmplt_epi32( ::bias_unsigned( x ), ::bias_unsigned( y ) );
#else
	auto X = vector( x ), Y = vector( y );
	return set(X[0] < Y[0], X[1] < Y[1], X[2] < Y[2], X[3] < Y[3]);
#endif
}
inline uf::simd::value<uint32_t> uf::simd::lessEquals(value<uint32_t> x, value<uint32_t> y) {
#if SSE_INSTR_SET >= 2
	// a <= b  <=>  !(a > b)
	__m128i bx = ::bias_unsigned(x);
	__m128i by = ::bias_unsigned(y);
	__m128i gt = _mm_cmpgt_epi32(bx, by); // signed compare
	return _mm_xor_si128(gt, _mm_set1_epi32(-1)); // invert mask
#else
	auto X = vector(x), Y = vector(y);
	return set(X[0] <= Y[0], X[1] <= Y[1], X[2] <= Y[2], X[3] <= Y[3]);
#endif
}
inline uf::simd::value<uint32_t> uf::simd::greater( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
#if SSE_INSTR_SET >= 4
	return _mm_cmpgt_epi32( ::bias_unsigned( x ), ::bias_unsigned( y ) );
#else
	auto X = vector( x ), Y = vector( y );
	return uf::simd::set(X[0] > Y[0], X[1] > Y[1], X[2] > Y[2], X[3] > Y[3]);
#endif
}
inline uf::simd::value<uint32_t> uf::simd::greaterEquals(value<uint32_t> x, value<uint32_t> y) {
#if SSE_INSTR_SET >= 2
	// a >= b  <=>  !(a < b)
	__m128i bx = ::bias_unsigned(x);
	__m128i by = ::bias_unsigned(y);
	__m128i lt = _mm_cmplt_epi32(bx, by); // signed compare
	return _mm_xor_si128(lt, _mm_set1_epi32(-1)); // invert mask
#else
	auto X = vector(x), Y = vector(y);
	return set(X[0] >= Y[0], X[1] >= Y[1], X[2] >= Y[2], X[3] >= Y[3]);
#endif
}
inline uf::simd::value<uint32_t> uf::simd::equals( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	return _mm_cmpeq_epi32(x, y);
}
inline uf::simd::value<uint32_t> uf::simd::notEquals( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	return _mm_xor_si128(_mm_cmpeq_epi32(x, y), _mm_set1_epi32(-1));
}
inline uf::simd::value<uint32_t> uf::simd::sqrt( uf::simd::value<uint32_t> v ) {
	auto V = uf::simd::vector( v );
	return uf::simd::set( (uint32_t) std::sqrt(V[0]), (uint32_t) std::sqrt(V[1]), (uint32_t) std::sqrt(V[2]), (uint32_t) std::sqrt(V[3]) );
}
inline uint32_t uf::simd::dot( uf::simd::value<uint32_t> x, uf::simd::value<uint32_t> y ) {
	auto X = uf::simd::vector( x );
	auto Y = uf::simd::vector( y );
	return X[0] * Y[0] + X[1] * Y[1] + X[2] * Y[2] + X[3] * Y[3];
}