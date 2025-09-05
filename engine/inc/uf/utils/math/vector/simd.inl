#include <uf/utils/memory/alignment.h>

namespace {
	FORCE_INLINE __m128i bias_unsigned(__m128i v) {
		const __m128i signbit = _mm_set1_epi32(0x80000000);
		return _mm_xor_si128(v, signbit);
	}

	FORCE_INLINE int32_t boolMask(bool b) {
		return b ? -1 : 0; // 0xFFFFFFFF for true, 0x00000000 for false
	}
}

#define MV_INSTR_SET_DEFAULT __attribute__((target("default")))
#define MV_INSTR_SET_2 __attribute__((target("sse2")))
#define MV_INSTR_SET_3 __attribute__((target("sse3")))
#define MV_INSTR_SET_4 __attribute__((target("ssse3")))
#define MV_INSTR_SET_5 __attribute__((target("sse4.1")))
#define MV_INSTR_SET_6 __attribute__((target("sse4.2")))
#define MV_INSTR_SET_7 __attribute__((target("avx")))

template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector() {}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(const T* f) : m(uf::simd::load(f)) {}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(T f) : m(uf::simd::set(f)) {}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(T f0, T f1, T f2, T f3) : m(uf::simd::set(f0,f1,f2,f3)) {}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(bool f0, bool f1, bool f2, bool f3) : m(uf::simd::set(f0,f1,f2,f3)) {}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(const value_type& rhs) : m(rhs) {}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(const vector& rhs) : m(rhs.m) {}

template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(const pod::Vector<T,1>& rhs) : vector((T) rhs[0]){}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(const pod::Vector<T,2>& rhs) : vector((T) rhs[0], (T) rhs[1], 0, 0){}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(const pod::Vector<T,3>& rhs) : vector((T) rhs[0], (T) rhs[1], (T) rhs[2], 0){}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::vector(const pod::Vector<T,4>& rhs) : vector((T) rhs[0], (T) rhs[1], (T) rhs[2], (T) rhs[3]){}

template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator+( const vector& rhs ) {
	return uf::simd::add( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator-( const vector& rhs ) {
	return uf::simd::sub( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator*( const vector& rhs ) {
	return uf::simd::mul( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator/( const vector& rhs ) {
	return uf::simd::div( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator<( const vector& rhs ) {
	return uf::simd::less( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator<=( const vector& rhs ) {
	return uf::simd::lessEquals( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator>( const vector& rhs ) {
	return uf::simd::greater( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator>=( const vector& rhs ) {
	return uf::simd::greaterEquals( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator==( const vector& rhs ) {
	return uf::simd::equals( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T> uf::simd::vector<T>::operator!=( const vector& rhs ) {
	return uf::simd::notEquals( *this, rhs );
}
template<typename T>
FORCE_INLINE uf::simd::vector<T>& uf::simd::vector<T>::operator=(const uf::simd::vector<T>::value_type& rhs) {
	m = rhs;
	return *this;
}
template<typename T>
FORCE_INLINE uf::simd::vector<T>& uf::simd::vector<T>::operator=(const vector& rhs) {
	m = rhs.m;
	return *this;
}
template<typename T>
FORCE_INLINE uf::simd::vector<T>& uf::simd::vector<T>::operator=(const pod::Vector<T,4>& rhs) {
	m = uf::simd::load(&rhs[0]);
	return *this;
}
template<typename T>
FORCE_INLINE uf::simd::vector<T>::operator uf::simd::vector<T>::value_type() const {
	return m;
}

template<typename T>
template<size_t N>
FORCE_INLINE uf::simd::vector<T>::operator pod::Vector<T,N>() const {
	return uf::simd::cast<N>(*this);
}

template<size_t N>
FORCE_INLINE pod::Vector<float,N> uf::simd::cast( const uf::simd::vector<float> v ){
	pod::Vector4f r;
	uf::simd::store( v, &r[0] );
	return uf::vector::cast<float,N>(r);
}
template<size_t N>
FORCE_INLINE pod::Vector<int32_t,N> uf::simd::cast( const uf::simd::vector<int32_t> v ){
	pod::Vector4i r;
	uf::simd::store( v, &r[0] );
	return uf::vector::cast<int32_t,N>(r);
}
template<size_t N>
FORCE_INLINE pod::Vector<uint32_t,N> uf::simd::cast( const uf::simd::vector<uint32_t> v ){
	pod::Vector4ui r;
	uf::simd::store( v, &r[0] );
	return uf::vector::cast<uint32_t,N>(r);
}

FORCE_INLINE uf::simd::vector<float> uf::simd::load( const float* f ) {
	// if ( uf::aligned(f, 16) ) return _mm_load_ps(f);
	return _mm_loadu_ps( f );
}
FORCE_INLINE void uf::simd::store( uf::simd::vector<float> v, float* f ) {
	/* if ( uf::aligned(f, 16) ) _mm_store_ps(f, v);
	else */ _mm_storeu_ps( f, v );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::set( float f ) {
	return _mm_set1_ps( f );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::set( float x, float y, float z, float w ) {
	return _mm_setr_ps( x, y, z, w );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::add( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_add_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::sub( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_sub_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::mul( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_mul_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::div( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_div_ps( x, y );
}
/*
FORCE_INLINE uf::simd::vector<float> uf::simd::hadd( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
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

FORCE_INLINE uf::simd::vector<float> uf::simd::min( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_min_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::max( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_max_ps( x, y );
}
FORCE_INLINE bool uf::simd::all( uf::simd::vector<float> mask) {
	return _mm_movemask_ps( mask ) == 0xF; // all 4 bits set
}
FORCE_INLINE bool uf::simd::any( uf::simd::vector<float> mask) {
	return _mm_movemask_ps( mask ) != 0x0; // any bit set
}
FORCE_INLINE uf::simd::vector<float> uf::simd::less( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_cmplt_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::lessEquals( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_cmple_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::greater( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_cmpgt_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::greaterEquals( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_cmpge_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::equals( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_cmpeq_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::notEquals( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_cmpneq_ps( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::sqrt( uf::simd::vector<float> v ) {
	return _mm_sqrt_ps( v );
}

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<float> dot_impl( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
		return uf::simd::mul( x, y );
	}
	MV_INSTR_SET_3
	uf::simd::vector<float> dot_impl( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
		__m128 mulRes = _mm_mul_ps(x, y);
		__m128 shufReg = _mm_movehdup_ps(mulRes);
		__m128 sumsReg = _mm_add_ps(mulRes, shufReg);
		shufReg = _mm_movehl_ps(shufReg, sumsReg);
		return _mm_add_ss(sumsReg, shufReg);
	}
	MV_INSTR_SET_5
	uf::simd::vector<float> dot_impl( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
		return _mm_dp_ps(x, y, 0xF1);
	}
}
FORCE_INLINE float uf::simd::dot( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return _mm_cvtss_f32( ::dot_impl( x, y ) );
}

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<int32_t> load_impl( const int32_t* f ) {
		return uf::simd::vector<int32_t>( f[0], f[1], f[2], f[3] );
	}
	MV_INSTR_SET_3
	uf::simd::vector<int32_t> load_impl( const int32_t* f ) {
		// if ( uf::aligned(f, 16) ) return _mm_load_si128(reinterpret_cast<const __m128i*>(f));
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(f));
	}
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::load( const int32_t* f ) {
	return ::load_impl( f );
}

namespace {
	MV_INSTR_SET_DEFAULT
	void store_impl( uf::simd::vector<int32_t> v, int32_t* f ) {
		union { __m128i x; int32_t y[4]; } kludge;
		kludge.x = v;
		f[0] = kludge.y[0];
		f[1] = kludge.y[1];
		f[2] = kludge.y[2];
		f[3] = kludge.y[3];
	}
	MV_INSTR_SET_3
	void store_impl( uf::simd::vector<int32_t> v, int32_t* f ) {
		/*if ( uf::aligned(f, 16) ) _mm_store_si128(reinterpret_cast<__m128i*>(f), v);
		else*/ _mm_storeu_si128(reinterpret_cast<__m128i*>(f), v);
	}
}
FORCE_INLINE void uf::simd::store( uf::simd::vector<int32_t> v, int32_t* f ) {
	return ::store_impl( v, f );
}


FORCE_INLINE uf::simd::vector<int32_t> uf::simd::set( int32_t f ) {
	return _mm_set1_epi32(f);
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::set( int32_t x, int32_t y, int32_t z, int32_t w ) {
	return _mm_setr_epi32(x, y, z, w);
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::add( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return _mm_add_epi32(x, y);
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::sub( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return _mm_sub_epi32(x, y);
}

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<int32_t> mul_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		auto X = uf::simd::cast(x);
		auto Y = uf::simd::cast(y);
		return uf::simd::set(X[0]*Y[0], X[1]*Y[1], X[2]*Y[2], X[3]*Y[3]);
	}
	MV_INSTR_SET_4
	uf::simd::vector<int32_t> mul_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		return _mm_mullo_epi32(x, y);
	}
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::mul( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return ::mul_impl( x, y );
}


FORCE_INLINE uf::simd::vector<int32_t> uf::simd::div( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	auto X = uf::simd::cast( x );
	auto Y = uf::simd::cast( y );
	return uf::simd::set( X[0] / Y[0], X[1] / Y[1], X[2] / Y[2], X[3] / Y[3] );
}
/*
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::hadd( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	auto X = uf::simd::cast( x );
	auto Y = uf::simd::cast( y );
	return uf::simd::set( X[0] + Y[0], X[1] + Y[1], X[2] + Y[2], X[3] + Y[3] );
}
*/
namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<int32_t> min_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		auto X = uf::simd::cast(x);
		auto Y = uf::simd::cast(y);
		return uf::simd::set(std::min(X[0],Y[0]), std::min(X[1],Y[1]), std::min(X[2],Y[2]), std::min(X[3],Y[3]));
	}
	MV_INSTR_SET_4
	uf::simd::vector<int32_t> min_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		return _mm_min_epi32(x, y);
	}

	MV_INSTR_SET_DEFAULT
	uf::simd::vector<int32_t> max_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		auto X = uf::simd::cast(x);
		auto Y = uf::simd::cast(y);
		return uf::simd::set(std::max(X[0],Y[0]), std::max(X[1],Y[1]), std::max(X[2],Y[2]), std::max(X[3],Y[3]));
	}
	MV_INSTR_SET_4
	uf::simd::vector<int32_t> max_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		return _mm_max_epi32(x, y);
	}
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::min( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return ::min_impl(x, y);
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::max( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return ::max_impl(x, y);
}


FORCE_INLINE bool uf::simd::all( uf::simd::vector<int32_t> mask) {
	return _mm_movemask_epi8( mask ) == 0xFFFF; // all 4 bits set
}
FORCE_INLINE bool uf::simd::any( uf::simd::vector<int32_t> mask) {
	return _mm_movemask_epi8( mask ) != 0x0; // any bit set
}

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<int32_t> less_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		auto X = uf::simd::cast( x ), Y = uf::simd::cast( y );
		return uf::simd::set_i(X[0] < Y[0], X[1] < Y[1], X[2] < Y[2], X[3] < Y[3]);
	}
	MV_INSTR_SET_4
	uf::simd::vector<int32_t> less_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		return _mm_cmplt_epi32( x, y );
	}

	MV_INSTR_SET_DEFAULT
	uf::simd::vector<int32_t> lessEquals_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		auto X = uf::simd::cast( x ), Y = uf::simd::cast( y );
		return uf::simd::set_i(X[0] <= Y[0], X[1] <= Y[1], X[2] <= Y[2], X[3] <= Y[3]);
	}
	MV_INSTR_SET_4
	uf::simd::vector<int32_t> lessEquals_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		__m128i gt = _mm_cmpgt_epi32(x, y);
		return _mm_xor_si128(gt, _mm_set1_epi32(-1));
	}

	MV_INSTR_SET_DEFAULT
	uf::simd::vector<int32_t> greater_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		auto X = uf::simd::cast( x ), Y = uf::simd::cast( y );
		return uf::simd::set_i(X[0] > Y[0], X[1] > Y[1], X[2] > Y[2], X[3] > Y[3]);
	}
	MV_INSTR_SET_4
	uf::simd::vector<int32_t> greater_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		return _mm_cmpgt_epi32(x, y);
	}

	MV_INSTR_SET_DEFAULT
	uf::simd::vector<int32_t> greaterEquals_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		auto X = uf::simd::cast( x ), Y = uf::simd::cast( y );
		return uf::simd::set_i(X[0] >= Y[0], X[1] >= Y[1], X[2] >= Y[2], X[3] >= Y[3]);
	}
	MV_INSTR_SET_4
	uf::simd::vector<int32_t> greaterEquals_impl( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
		__m128i gt = _mm_cmplt_epi32(x, y);
		return _mm_xor_si128(gt, _mm_set1_epi32(-1));
	}
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::less( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return ::less_impl( x, y );
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::lessEquals( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return ::lessEquals_impl( x, y );
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::greater( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return ::greater_impl( x, y );
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::greaterEquals( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return ::greaterEquals_impl( x, y );
}

FORCE_INLINE uf::simd::vector<int32_t> uf::simd::equals( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return _mm_cmpeq_epi32(x, y);
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::notEquals( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	return _mm_xor_si128(_mm_cmpeq_epi32(x, y), _mm_set1_epi32(-1));
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::sqrt( uf::simd::vector<int32_t> v ) {
	auto V = uf::simd::cast( v );
	return uf::simd::set( (int32_t) std::sqrt(V[0]), (int32_t) std::sqrt(V[1]), (int32_t) std::sqrt(V[2]), (int32_t) std::sqrt(V[3]) );
}
FORCE_INLINE int32_t uf::simd::dot( uf::simd::vector<int32_t> x, uf::simd::vector<int32_t> y ) {
	auto X = uf::simd::cast( x );
	auto Y = uf::simd::cast( y );
	return X[0] * Y[0] + X[1] * Y[1] + X[2] * Y[2] + X[3] * Y[3];
}

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<uint32_t> load_impl( const uint32_t* f ) {
		return uf::simd::vector<uint32_t>( f[0], f[1], f[2], f[3] );
	}
	MV_INSTR_SET_3
	uf::simd::vector<uint32_t> load_impl( const uint32_t* f ) {
		// if ( uf::aligned(f, 16) ) return _mm_load_si128(reinterpret_cast<const __m128i*>(f));
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(f));
	}

	MV_INSTR_SET_DEFAULT
	void store_impl( uf::simd::vector<uint32_t> v, uint32_t* f ) {
		union { __m128i x; uint32_t y[4]; } kludge;
		kludge.x = v;
		f[0] = kludge.y[0];
		f[1] = kludge.y[1];
		f[2] = kludge.y[2];
		f[3] = kludge.y[3];
	}
	MV_INSTR_SET_3
	void store_impl( uf::simd::vector<uint32_t> v, uint32_t* f ) {
		/*if ( uf::aligned(f, 16) ) _mm_store_si128(reinterpret_cast<__m128i*>(f), v);
		else*/ _mm_storeu_si128(reinterpret_cast<__m128i*>(f), v);
	}
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::load( const uint32_t* f ) {
	return ::load_impl( f );
}
FORCE_INLINE void uf::simd::store( uf::simd::vector<uint32_t> v, uint32_t* f ) {
	return ::store_impl( v, f );
}

FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::set( uint32_t f ) {
	return _mm_set1_epi32(f);
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::set( uint32_t x, uint32_t y, uint32_t z, uint32_t w ) {
	return _mm_setr_epi32(x, y, z, w);
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::add( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return _mm_add_epi32(x, y);
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::sub( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return _mm_sub_epi32(x, y);
}

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<uint32_t> mul_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		auto X = uf::simd::cast(x);
		auto Y = uf::simd::cast(y);
		return uf::simd::set(X[0]*Y[0], X[1]*Y[1], X[2]*Y[2], X[3]*Y[3]);
	}
	MV_INSTR_SET_4
	uf::simd::vector<uint32_t> mul_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		return _mm_mullo_epi32(x, y);
	}
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::mul( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return ::mul_impl( x, y );
}

FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::div( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	auto X = uf::simd::cast( x );
	auto Y = uf::simd::cast( y );
	return uf::simd::set( X[0] / Y[0], X[1] / Y[1], X[2] / Y[2], X[3] / Y[3] );
}
/*
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::hadd( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	auto X = uf::simd::cast( x );
	auto Y = uf::simd::cast( y );
	return uf::simd::set( X[0] + Y[0], X[1] + Y[1], X[2] + Y[2], X[3] + Y[3] );
}
*/

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<uint32_t> min_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		auto X = uf::simd::cast(x);
		auto Y = uf::simd::cast(y);
		return uf::simd::set(std::min(X[0],Y[0]), std::min(X[1],Y[1]), std::min(X[2],Y[2]), std::min(X[3],Y[3]));
	}
	MV_INSTR_SET_4
	uf::simd::vector<uint32_t> min_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		return _mm_min_epu32(x, y); // unsigned min
	}

	MV_INSTR_SET_DEFAULT
	uf::simd::vector<uint32_t> max_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		auto X = uf::simd::cast(x);
		auto Y = uf::simd::cast(y);
		return uf::simd::set(std::max(X[0],Y[0]), std::max(X[1],Y[1]), std::max(X[2],Y[2]), std::max(X[3],Y[3]));
	}
	MV_INSTR_SET_4
	uf::simd::vector<uint32_t> max_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		return _mm_max_epu32(x, y); // unsigned max
	}
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::min( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return ::min_impl(x, y);
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::max( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return ::max_impl(x, y);
}

FORCE_INLINE bool uf::simd::all( uf::simd::vector<uint32_t> mask) {
	return _mm_movemask_epi8( mask ) == 0xFFFF; // all 4 bits set
}
FORCE_INLINE bool uf::simd::any( uf::simd::vector<uint32_t> mask) {
	return _mm_movemask_epi8( mask ) != 0x0; // any bit set
}

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<uint32_t> less_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		auto X = uf::simd::cast( x ), Y = uf::simd::cast( y );
		return uf::simd::set_ui(X[0] < Y[0], X[1] < Y[1], X[2] < Y[2], X[3] < Y[3]);
	}
	MV_INSTR_SET_4
	uf::simd::vector<uint32_t> less_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		return _mm_cmplt_epi32( ::bias_unsigned(x), ::bias_unsigned(y) );
	}

	MV_INSTR_SET_DEFAULT
	uf::simd::vector<uint32_t> lessEquals_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y) {
		auto X = uf::simd::cast(x), Y = uf::simd::cast(y);
		return uf::simd::set_ui(X[0] <= Y[0], X[1] <= Y[1], X[2] <= Y[2], X[3] <= Y[3]);
	}
	MV_INSTR_SET_2
	uf::simd::vector<uint32_t> lessEquals_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y) {
		// a <= b  <=>  !(a > b)
		__m128i bx = ::bias_unsigned(x);
		__m128i by = ::bias_unsigned(y);
		__m128i gt = _mm_cmpgt_epi32(bx, by); // signed compare
		return _mm_xor_si128(gt, _mm_set1_epi32(-1)); // invert mask
	}

	MV_INSTR_SET_DEFAULT
	uf::simd::vector<uint32_t> greater_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		auto X = uf::simd::cast( x ), Y = uf::simd::cast( y );
		return uf::simd::set_ui(X[0] > Y[0], X[1] > Y[1], X[2] > Y[2], X[3] > Y[3]);
	}
	MV_INSTR_SET_4
	uf::simd::vector<uint32_t> greater_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
		return _mm_cmpgt_epi32( ::bias_unsigned(x), ::bias_unsigned(y) );
	}

	MV_INSTR_SET_DEFAULT
	uf::simd::vector<uint32_t> greaterEquals_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y) {
		auto X = uf::simd::cast(x), Y = uf::simd::cast(y);
		return uf::simd::set_ui(X[0] >= Y[0], X[1] >= Y[1], X[2] >= Y[2], X[3] >= Y[3]);
	}
	MV_INSTR_SET_2
	uf::simd::vector<uint32_t> greaterEquals_impl( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y) {
		// a >= b  <=>  !(a < b)
		__m128i bx = ::bias_unsigned(x);
		__m128i by = ::bias_unsigned(y);
		__m128i lt = _mm_cmplt_epi32(bx, by); // signed compare
		return _mm_xor_si128(lt, _mm_set1_epi32(-1)); // invert mask
	}
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::less( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return ::less_impl( x, y );
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::lessEquals( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return ::lessEquals_impl( x, y );
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::greater( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return ::greater_impl( x, y );
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::greaterEquals( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return ::greaterEquals_impl( x, y );
}


FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::equals( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return _mm_cmpeq_epi32(x, y);
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::notEquals( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	return _mm_xor_si128(_mm_cmpeq_epi32(x, y), _mm_set1_epi32(-1));
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::sqrt( uf::simd::vector<uint32_t> v ) {
	auto V = uf::simd::cast( v );
	return uf::simd::set( (uint32_t) std::sqrt(V[0]), (uint32_t) std::sqrt(V[1]), (uint32_t) std::sqrt(V[2]), (uint32_t) std::sqrt(V[3]) );
}
FORCE_INLINE uint32_t uf::simd::dot( uf::simd::vector<uint32_t> x, uf::simd::vector<uint32_t> y ) {
	auto X = uf::simd::cast( x );
	auto Y = uf::simd::cast( y );
	return X[0] * Y[0] + X[1] * Y[1] + X[2] * Y[2] + X[3] * Y[3];
}

FORCE_INLINE uf::simd::vector<float> uf::simd::set_f( bool x, bool y, bool z, bool w ) {
	return _mm_castsi128_ps(_mm_setr_epi32(::boolMask(x), ::boolMask(y), ::boolMask(z), ::boolMask(w)));
}
FORCE_INLINE uf::simd::vector<int32_t> uf::simd::set_i( bool x, bool y, bool z, bool w ) {
	return _mm_setr_epi32(::boolMask(x), ::boolMask(y), ::boolMask(z), ::boolMask(w));
}
FORCE_INLINE uf::simd::vector<uint32_t> uf::simd::set_ui( bool x, bool y, bool z, bool w ) {
	return _mm_setr_epi32(::boolMask(x), ::boolMask(y), ::boolMask(z), ::boolMask(w));
}

namespace {
	MV_INSTR_SET_DEFAULT
	uf::simd::vector<float> cross_impl( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
		__m128 tmp0 = _mm_shuffle_ps(y,y,_MM_SHUFFLE(3,0,2,1));
		__m128 tmp1 = _mm_shuffle_ps(x,x,_MM_SHUFFLE(3,0,2,1));
		tmp0 = _mm_mul_ps(tmp0,x);
		tmp1 = _mm_mul_ps(tmp1,y);
		__m128 tmp2 = _mm_sub_ps(tmp0,tmp1);
		__m128 res = _mm_shuffle_ps(tmp2,tmp2,_MM_SHUFFLE(3,0,2,1));
		return res;
	}
	MV_INSTR_SET_7
	uf::simd::vector<float> cross_impl( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
		__m128 tmp0 = _mm_shuffle_ps(y,y,_MM_SHUFFLE(3,0,2,1));
		__m128 tmp1 = _mm_shuffle_ps(x,x,_MM_SHUFFLE(3,0,2,1));
		tmp1 = _mm_mul_ps(tmp1,y);
		__m128 tmp2 = _mm_fmsub_ps( tmp0,x, tmp1 );
		__m128 res = _mm_shuffle_ps(tmp2,tmp2,_MM_SHUFFLE(3,0,2,1));
		return res;
	}
}

FORCE_INLINE uf::simd::vector<float> uf::simd::cross( uf::simd::vector<float> x, uf::simd::vector<float> y ) {
	return ::cross_impl( x, y );
}
FORCE_INLINE uf::simd::vector<float> uf::simd::normalize( uf::simd::vector<float> v ) {
	__m128 len = _mm_sqrt_ss( ::dot_impl( v,v ) );
	len = _mm_shuffle_ps(len, len, 0x00);
	return _mm_div_ps(v, len);
}
FORCE_INLINE uf::simd::vector<float> uf::simd::normalize_fast( uf::simd::vector<float> v ) {
	__m128 invLen = _mm_rsqrt_ps(::dot_impl(v, v));
	return _mm_mul_ps(v, invLen);
}