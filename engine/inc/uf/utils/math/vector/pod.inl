#include <uf/config.h>
#include <cstring> // memcmp

// 	Equality checking
template<typename T>
pod::Vector1t<T> /*UF_API*/ uf::vector::create( T x ) { pod::Vector1t<T> vec; vec.x = x; return vec; }
template<typename T>
pod::Vector2t<T> /*UF_API*/ uf::vector::create( T x, T y ) { pod::Vector2t<T> vec; vec.x = x, vec.y = y; return vec; }
template<typename T>
pod::Vector3t<T> /*UF_API*/ uf::vector::create( T x, T y, T z ) { pod::Vector3t<T> vec; vec.x = x, vec.y = y, vec.z = z; return vec; }
template<typename T>
pod::Vector4t<T> /*UF_API*/ uf::vector::create( T x, T y, T z, T w ) { pod::Vector4t<T> vec; vec.x = x, vec.y = y, vec.z = z, vec.w = w; return vec; }
template<typename T, size_t N>
pod::Vector<T, N> /*UF_API*/ uf::vector::copy( const pod::Vector<T, N>& v ) { return v; }
// 	Equality checking
template<typename T> 														// 	Equality check between two vectors (less than)
std::size_t /*UF_API*/ uf::vector::compareTo( const T& left, const T& right ) {
	return memcmp( &left, &right, T::size );
}
template<typename T> 														// 	Equality check between two vectors (equals)
bool /*UF_API*/ uf::vector::equals( const T& left, const T& right ) {
//	return uf::vector::compareTo(left, right) == 0;
	for ( std::size_t i = 0; i < T::size; ++i ) if ( left[i] != right[i] ) return false;
	return true;
}
// Basic arithmetic
template<typename T> 														// Adds two vectors of same type and size together
T /*UF_API*/ uf::vector::add( const T& left, const T& right ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = left[i] + right[i];
	return res;
}
template<typename T> 														// Subtracts two vectors of same type and size together
T /*UF_API*/ uf::vector::subtract( const T& left, const T& right ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = left[i] - right[i];
	return res;
}
template<typename T> 														// Multiplies two vectors of same type and size together
T /*UF_API*/ uf::vector::multiply( const T& left, const T& right ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = left[i] * right[i];
	return res;
}
template<typename T> 														// Multiplies this vector by a scalar
T /*UF_API*/ uf::vector::multiply( const T& vector, const typename T::type_t& scalar ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = vector[i] * scalar;
	return res;
}
template<typename T> 														// Divides two vectors of same type and size together
T /*UF_API*/ uf::vector::divide( const T& left, const T& right ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = left[i] / right[i];
	return res;
}
template<typename T> 														// Divides this vector by a scalar
T /*UF_API*/ uf::vector::divide( const T& vector, const typename T::type_t& scalar ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = vector[i] / scalar;
	return res;
}
template<typename T> 														// Compute the sum of all components 
typename T::type_t /*UF_API*/ uf::vector::sum( const T& vector ) {
	typename T::type_t res = 0;
	for ( std::size_t i = 0; i < T::size; ++i ) res += vector[i];
	return res;
}
template<typename T> 														// Compute the product of all components 
typename T::type_t /*UF_API*/ uf::vector::product( const T& vector ) {
	typename T::type_t res = 0;
	for ( std::size_t i = 0; i < T::size; ++i ) res *= vector[i];
	return res;
}
template<typename T> 														// Flip sign of all components
T /*UF_API*/ uf::vector::negate( const T& vector ) {
	T res;
	for ( std::size_t i = 0; i < T::size; ++i ) res[i] = -vector[i];
	return res;
}
// Writes to first value
template<typename T> 														// Adds two vectors of same type and size together
T& /*UF_API*/ uf::vector::add( T& left, const T& right ) {
	for ( std::size_t i = 0; i < T::size; ++i ) left[i] += right[i];
	return left;
}
template<typename T> 														// Subtracts two vectors of same type and size together
T& /*UF_API*/ uf::vector::subtract( T& left, const T& right ) {
	for ( std::size_t i = 0; i < T::size; ++i ) left[i] -= right[i];
	return left;
}
template<typename T> 														// Multiplies two vectors of same type and size together
T& /*UF_API*/ uf::vector::multiply( T& left, const T& right ) {
	for ( std::size_t i = 0; i < T::size; ++i ) left[i] *= right[i];
	return left;
}
template<typename T> 														// Multiplies this vector by a scalar
T& /*UF_API*/ uf::vector::multiply( T& vector, const typename T::type_t& scalar ) {
	for ( std::size_t i = 0; i < T::size; ++i ) vector[i] *= scalar;
	return vector;
}
template<typename T> 														// Divides two vectors of same type and size together
T& /*UF_API*/ uf::vector::divide( T& left, const T& right ) {
	for ( std::size_t i = 0; i < T::size; ++i ) left[i] /= right[i];
	return left;
}
template<typename T> 														// Divides this vector by a scalar
T& /*UF_API*/ uf::vector::divide( T& vector, const typename T::type_t& scalar ) {
	for ( std::size_t i = 0; i < T::size; ++i ) vector[i] /= scalar;
	return vector;
}
template<typename T> 														// Flip sign of all components
T& /*UF_API*/ uf::vector::negate( T& vector ) {
	for ( std::size_t i = 0; i < T::size; ++i ) vector[i] = -vector[i];
	return vector;
}
template<typename T> 														// Normalizes a vector
T& /*UF_API*/ uf::vector::normalize( T& vector ) {
	typename T::type_t norm = uf::vector::norm(vector);
	if ( norm == 0 ) return vector;	
	return uf::vector::divide(vector, norm);
}
// Complex arithmetic
template<typename T> 														// Compute the dot product between two vectors
typename T::type_t /*UF_API*/ uf::vector::dot( const T& left, const T& right ) {
/*
	typename T::type_t dot = 0;
	for ( std::size_t i = 0; i < T::size; ++i ) dot += left[i] * right[i];
	return dot;
*/
	return uf::vector::sum(uf::vector::multiply(left, right));
}
template<typename T> 														// Compute the angle between two vectors
pod::Angle /*UF_API*/ uf::vector::angle( const T& a, const T& b ) {
	return acos(uf::vector::dot(a, b));
}
template<typename T> 														// Linearly interpolate between two vectors
T /*UF_API*/ uf::vector::lerp( const T& from, const T& to, double delta ) {
	delta = fmax( 0, fmin(1,delta) );
	// from + ( ( to - from ) * delta )
	return uf::vector::add(from, uf::vector::multiply( uf::vector::subtract(to, from), delta ) );
}
template<typename T> 														// 
T /*UF_API*/ uf::vector::mix( const T& x, const T& y, double a ) {
	// delta = fmax( 0, fmin(1,delta) );
	// x * (1.0 - a) + y * a
	return uf::vector::add( uf::vector::multiply( x, 1 - a ), uf::vector::multiply( y, a ) );
}
template<typename T> 														// Spherically interpolate between two vectors
T /*UF_API*/ uf::vector::slerp( const T& from, const T& to, double delta ) {
	//delta = fmax( 0, fmin(1,delta) );
	typename T::type_t dot = uf::vector::dot(from, to);
	typename T::type_t theta = acos(dot);
	typename T::type_t sTheta = sin(theta);

	typename T::type_t w1 = sin((1.0f - delta) * theta / sTheta);
	typename T::type_t w2 = sin( delta * theta / sTheta );

	return uf::vector::add(uf::vector::multiply(from, w1), uf::vector::multiply(to, w2));
}
template<typename T> 														// Compute the distance between two vectors (doesn't sqrt)
typename T::type_t /*UF_API*/ uf::vector::distanceSquared( const T& a, const T& b ) {
	T delta = uf::vector::subtract(b, a);
	uf::vector::multiply( delta, delta );
	return uf::vector::sum(delta);
}
template<typename T> 														// Compute the distance between two vectors
typename T::type_t /*UF_API*/ uf::vector::distance( const T& a, const T& b ) {
	return sqrt(uf::vector::distanceSquared(a,b));
}
template<typename T> 														// Gets the magnitude of the vector
typename T::type_t /*UF_API*/ uf::vector::magnitude( const T& vector ) {
	return uf::vector::dot(vector, vector);
}
template<typename T> 														// Compute the norm of the vector
typename T::type_t /*UF_API*/ uf::vector::norm( const T& vector ) {
	return sqrt( uf::vector::magnitude(vector) );
}
template<typename T> 														// Normalizes a vector
T /*UF_API*/ uf::vector::normalize( const T& vector ) {
	typename T::type_t norm = uf::vector::norm(vector);
	if ( norm == 0 ) return vector;	
	return uf::vector::divide(vector, norm);
}
template<typename T> 														// Normalizes a vector
T /*UF_API*/ uf::vector::cross( const T& a, const T& b ) {
	return {
		a.y * b.z - b.y * a.z,
		a.z * b.x - b.z * a.x,
		a.x * b.y - b.x * a.y
	};
}
template<typename T> 														// Normalizes a vector
std::string /*UF_API*/ uf::vector::toString( const T& vector ) {
	std::stringstream str;
	str << "Vector(";
	for ( std::size_t i = 0; i < vector.size; ++i ) {
		str << vector[i] << ( i + 1 < vector.size ? ", " : "" );
	}
	str << ")";
	return str.str();
}





//
template<typename T, std::size_t N>
T& pod::Vector<T,N>::operator[](std::size_t i) {
	return this->components[i];
}
template<typename T, std::size_t N>
const T& pod::Vector<T,N>::operator[](std::size_t i) const {
	return this->components[i];
}
// Arithmetic
template<typename T, std::size_t N> 												// 	Negation
inline pod::Vector<T,N> pod::Vector<T,N>::operator-() const {
	return uf::vector::negate( *this );
}			
template<typename T, std::size_t N> 												// 	Addition between two vectors
inline pod::Vector<T,N> pod::Vector<T,N>::operator+( const pod::Vector<T,N>& vector ) const {
	return uf::vector::add( *this, vector );
}
template<typename T, std::size_t N> 												// 	Subtraction between two vectors
inline pod::Vector<T,N> pod::Vector<T,N>::operator-( const pod::Vector<T,N>& vector ) const {
	return uf::vector::subtract( *this, vector );
}
template<typename T, std::size_t N> 												// 	Multiplication between two vectors
inline pod::Vector<T,N> pod::Vector<T,N>::operator*( const pod::Vector<T,N>& vector ) const {
	return uf::vector::multiply( *this, vector );
}
template<typename T, std::size_t N> 												// 	Division between two vectors
inline pod::Vector<T,N> pod::Vector<T,N>::operator/( const pod::Vector<T,N>& vector ) const {
	return uf::vector::divide( *this, vector );
}
template<typename T, std::size_t N> 												// 	Multiplication with scalar
inline pod::Vector<T,N> pod::Vector<T,N>::operator*( T scalar ) const {
	return uf::vector::multiply( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Division with scalar
inline pod::Vector<T,N> pod::Vector<T,N>::operator/( T scalar ) const {
	return uf::vector::divide( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Addition set between two vectors
inline pod::Vector<T,N>& pod::Vector<T,N>::operator +=( const pod::Vector<T,N>& vector ) {
	return uf::vector::add( *this, vector );
}
template<typename T, std::size_t N> 												// 	Subtraction set between two vectors
inline pod::Vector<T,N>& pod::Vector<T,N>::operator -=( const pod::Vector<T,N>& vector ) {
	return uf::vector::subtract( *this, vector );
}
template<typename T, std::size_t N> 												// 	Multiplication set between two vectors
inline pod::Vector<T,N>& pod::Vector<T,N>::operator *=( const pod::Vector<T,N>& vector ) {
	return uf::vector::multiply( *this, vector );
}
template<typename T, std::size_t N> 												// 	Division set between two vectors
inline pod::Vector<T,N>& pod::Vector<T,N>::operator /=( const pod::Vector<T,N>& vector ) {
	return uf::vector::divide( *this, vector );
}
template<typename T, std::size_t N> 												// 	Multiplication set with scalar
inline pod::Vector<T,N>& pod::Vector<T,N>::operator *=( T scalar ) {
	return uf::vector::multiply( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Division set with scalar
inline pod::Vector<T,N>& pod::Vector<T,N>::operator /=( T scalar ) {
	return uf::vector::divide( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (equals)
inline bool pod::Vector<T,N>::operator==( const pod::Vector<T,N>& vector ) const {
	return uf::vector::equals(*this, vector);
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (not equals)
inline bool pod::Vector<T,N>::operator!=( const pod::Vector<T,N>& vector ) const {
	return !uf::vector::equals(*this, vector);
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (less than)
inline bool pod::Vector<T,N>::operator<( const pod::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(vector) < 0;
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (less than or equals)
inline bool pod::Vector<T,N>::operator<=( const pod::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(vector) <= 0;
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (greater than)
inline bool pod::Vector<T,N>::operator>( const pod::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(vector) > 0;
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (greater than or equals)
inline bool pod::Vector<T,N>::operator>=( const pod::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(vector) >= 0;
}
#include "redundancy.inl"