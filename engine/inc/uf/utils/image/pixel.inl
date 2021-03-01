#include <uf/config.h>
#include <cstring> // memcmp

// 	Equality checking
template<typename T> 														// 	Equality check between two pixels (less than)
std::size_t /*UF_API*/ uf::pixel::compareTo( const T& left, const T& right ) {
	return memcmp( &left, &right, T::size );
}
template<typename T> 														// 	Equality check between two pixels (equals)
bool /*UF_API*/ uf::pixel::equals( const T& left, const T& right ) {
	return !compareTo(left, right);
}
// Basic arithmetic
template<typename T> 														// Multiplies two pixels of same type and size together
T /*UF_API*/ uf::pixel::multiply( const T& left, const T& right ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = left[i] * right[i];
	return res;
}
template<typename T> 														// Multiplies this pixel by a scalar
T /*UF_API*/ uf::pixel::multiply( const T& pixel, const typename T::type_t& scalar ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = pixel[i] * scalar;
	return res;
}
template<typename T> 														// Divides two pixels of same type and size together
T /*UF_API*/ uf::pixel::divide( const T& left, const T& right ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = left[i] / right[i];
	return res;
}
template<typename T> 														// Divides this pixel by a scalar
T /*UF_API*/ uf::pixel::divide( const T& pixel, const typename T::type_t& scalar ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = pixel[i] / scalar;
	return res;
}
template<typename T> 														// Invert the color
T /*UF_API*/ uf::pixel::negate( const T& pixel ) {
	typename T::type_t max = 0;
	max = ~0;
	typename T::type_t res = 0;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = max - res[i];
	return res;
}
// Writes to first value
template<typename T> 														// Multiplies two pixels of same type and size together
T& /*UF_API*/ uf::pixel::multiply( T& left, const T& right ) {
	for ( std::size_t i = 0; i < T::size; ++i ) left[i] *= right[i];
	return left;
}
template<typename T> 														// Multiplies this pixel by a scalar
T& /*UF_API*/ uf::pixel::multiply( T& pixel, const typename T::type_t& scalar ) {
	for ( std::size_t i = 0; i < T::size; ++i ) pixel[i] * scalar;
	return pixel;
}
template<typename T> 														// Divides two pixels of same type and size together
T& /*UF_API*/ uf::pixel::divide( T& left, const T& right ) {
	for ( std::size_t i = 0; i < T::size; ++i ) left[i] /= right[i];
	return left;
}
template<typename T> 														// Divides this pixel by a scalar
T& /*UF_API*/ uf::pixel::divide( T& pixel, const typename T::type_t& scalar ) {
	for ( std::size_t i = 0; i < T::size; ++i ) pixel[i] /= scalar;
	return pixel;
}
template<typename T> 														// Flip sign of all components
T& /*UF_API*/ uf::pixel::negate( T& pixel ) {
	for ( std::size_t i = 0; i < T::size; ++i ) pixel[i] = -pixel[i];
	return pixel;
}


//
template<typename T, std::size_t N>
T& pod::Pixel<T,N>::operator[](std::size_t i) {
	return this->components[i];
}
template<typename T, std::size_t N>
const T& pod::Pixel<T,N>::operator[](std::size_t i) const {
	return this->components[i];
}
// Arithmetic
template<typename T, std::size_t N> 												// 	Negation
inline pod::Pixel<T,N> pod::Pixel<T,N>::operator-() const {
	return uf::pixel::negate( *this );
}
template<typename T, std::size_t N> 												// 	Multiplication between two pixels
inline pod::Pixel<T,N> pod::Pixel<T,N>::operator*( const pod::Pixel<T,N>& pixel ) const {
	return uf::pixel::multiply( *this, pixel );
}
template<typename T, std::size_t N> 												// 	Division between two pixels
inline pod::Pixel<T,N> pod::Pixel<T,N>::operator/( const pod::Pixel<T,N>& pixel ) const {
	return uf::pixel::divide( *this, pixel );
}
template<typename T, std::size_t N> 												// 	Multiplication with scalar
inline pod::Pixel<T,N> pod::Pixel<T,N>::operator*( T scalar ) const {
	return uf::pixel::multiply( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Division with scalar
inline pod::Pixel<T,N> pod::Pixel<T,N>::operator/( T scalar ) const {
	return uf::pixel::divide( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Multiplication set between two pixels
inline pod::Pixel<T,N>& pod::Pixel<T,N>::operator *=( const pod::Pixel<T,N>& pixel ) {
	return uf::pixel::multiply( *this, pixel );
}
template<typename T, std::size_t N> 												// 	Division set between two pixels
inline pod::Pixel<T,N>& pod::Pixel<T,N>::operator /=( const pod::Pixel<T,N>& pixel ) {
	return uf::pixel::divide( *this, pixel );
}
template<typename T, std::size_t N> 												// 	Multiplication set with scalar
inline pod::Pixel<T,N>& pod::Pixel<T,N>::operator *=( T scalar ) {
	return uf::pixel::multiply( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Division set with scalar
inline pod::Pixel<T,N>& pod::Pixel<T,N>::operator /=( T scalar ) {
	return uf::pixel::divide( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Equality check between two pixels (equals)
inline bool pod::Pixel<T,N>::operator==( const pod::Pixel<T,N>& pixel ) const {
	return uf::pixel::equals(pixel);
}
template<typename T, std::size_t N> 												// 	Equality check between two pixels (not equals)
inline bool pod::Pixel<T,N>::operator!=( const pod::Pixel<T,N>& pixel ) const {
	return !uf::pixel::equals(pixel);
}
#include "redundancy.inl"