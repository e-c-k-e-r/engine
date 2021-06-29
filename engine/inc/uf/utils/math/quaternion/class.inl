// 	C-tor
// initializes POD to 'def'
template<typename T>
uf::Quaternion<T>::Quaternion() {

}
// initializes POD to 'def'
template<typename T>
uf::Quaternion<T>::Quaternion(T def) :
	m_pod( {def, def, def, 1} )
{

}
// initializes POD to 'def'
template<typename T>
uf::Quaternion<T>::Quaternion(T x, T y, T z, T w) :
	m_pod( {x, y, z, w} )
{

}
// copies POD altogether
template<typename T>
uf::Quaternion<T>::Quaternion(const Quaternion<T>::pod_t& pod) :
	m_pod(pod)
{

}
// copies data into POD from 'components' (typed as C array)
template<typename T>
uf::Quaternion<T>::Quaternion(const T components[4] ) : 
	m_pod( { components[0], components[1], components[2], components[3] } )
{

}
// copies data into POD from 'components' (typed as uf::stl::vector<T>)
template<typename T>
uf::Quaternion<T>::Quaternion(const uf::stl::vector<T>& components) {
	memcpy( this->m_pod.components, &components[0], 4 );
}
// 	D-tor
// Unneccesary
// 	POD access
// 	Returns a reference of POD
template<typename T>
typename uf::Quaternion<T>::pod_t& uf::Quaternion<T>::data() {
	return this->m_pod;
}
// 	Returns a const-reference of POD
template<typename T>
const typename uf::Quaternion<T>::pod_t& uf::Quaternion<T>::data() const {
	return this->m_pod;
}
// 	Alternative POD access
// 	Returns a pointer to the entire array
template<typename T>
T* uf::Quaternion<T>::get() {
	return this->m_pod.components;
}
// 	Returns a const-pointer to the entire array
template<typename T>
const T* uf::Quaternion<T>::get() const {
	return this->m_pod.components;
}
// 	Returns a reference to a single element
template<typename T>
T& uf::Quaternion<T>::getComponent( std::size_t i ) {
	return this->m_pod.components[i];
}
// 	Returns a const-reference to a single element
template<typename T>
const T& uf::Quaternion<T>::getComponent( std::size_t i ) const {
	return this->m_pod.components[i];
}
// 	POD manipulation
// 	Sets the entire array
template<typename T>
T* uf::Quaternion<T>::set(const T components[4]) {
	for ( std::size_t i = 0; i < 4; ++i ) this->m_pod[i] = components[i];
}
// 	Sets a single element
template<typename T>
T& uf::Quaternion<T>::setComponent( std::size_t i, const T& value ) {
	this->m_pod[i] = value;
}
// 	Validation
// 	Checks if all components are valid (non NaN, inf, etc.)
template<typename T>
bool uf::Quaternion<T>::isValid() const {
	for ( std::size_t i = 0; i < 4; ++i ) if ( this->m_pod[i] != this->m_pod[i] ) return false;
	return true;
}
// 	Basic arithmetic
// 	Multiplies two quaternions of same type and size together
template<typename T>
inline uf::Quaternion<T>& uf::Quaternion<T>::multiply( const Quaternion<T>& b ) {
	uf::quaternion::multiply(this->m_pod, b.m_pod);
	return *this;
}
template<typename T>
inline uf::Quaternion<T>  uf::Quaternion<T>::multiply( const Quaternion<T>& b ) const {
	return uf::quaternion::multiply(this->m_pod, b.m_pod);
}
template<typename T>
inline uf::Vector3t<T>  uf::Quaternion<T>::rotate( const Vector3t<T>& b ) const {
	return uf::quaternion::rotate(this->m_pod, b.data());
}
template<typename T>
inline uf::Vector4t<T>  uf::Quaternion<T>::rotate( const Vector4t<T>& b ) const {
	return uf::quaternion::rotate(this->m_pod, b.data());
}
// 	Flip sign of all components
template<typename T>
inline uf::Quaternion<T>& uf::Quaternion<T>::negate() {
	uf::quaternion::negate(this->m_pod);
	return *this;
}
// 	Complex arithmetic
// 	Compute the dot product between two quaternions
template<typename T>
inline T uf::Quaternion<T>::dot( const Quaternion<T> right ) const {
	return uf::quaternion::dot( this->m_pod, right.m_pod );
}
// 	Compute the angle between two quaternions
template<typename T>
inline pod::Angle uf::Quaternion<T>::angle( const Quaternion<T>& b ) const {
	return uf::quaternion::angle( this->m_pod, b.m_pod );
}

// 	Linearly interpolate between two quaternions
template<typename T>
inline uf::Quaternion<T> uf::Quaternion<T>::lerp( const Quaternion<T> to, typename T::type_t delta ) const {
	return uf::quaternion::lerp( this->m_pod, to.m_pod, delta );
}
// 	Spherically interpolate between two quaternions
template<typename T>
inline uf::Quaternion<T> uf::Quaternion<T>::slerp( const Quaternion<T> to, typename T::type_t delta ) const {
	return uf::quaternion::slerp( this->m_pod, to.m_pod, delta );
}

// 	Compute the distance between two quaternions (doesn't sqrt)
template<typename T>
inline T uf::Quaternion<T>::distanceSquared( const Quaternion<T> b ) const {
	return uf::quaternion::distanceSquared(this->m_pod, b.m_pod);
}
// 	Compute the distance between two quaternions
template<typename T>
inline T uf::Quaternion<T>::distance( const Quaternion<T> b ) const {
	return uf::quaternion::distance(this->m_pod, b.m_pod);
}
// 	Gets the magnitude of the quaternion
template<typename T>
inline T uf::Quaternion<T>::magnitude() const {
	return uf::quaternion::magnitude(this->m_pod);
}
// 	Compute the norm of the quaternion
template<typename T>
inline T uf::Quaternion<T>::norm() const {
	return uf::quaternion::norm(this->m_pod);
}

// 	Normalizes a quaternion
template<typename T>
inline uf::Quaternion<T>& uf::Quaternion<T>::normalize() {
	uf::quaternion::normalize(this->m_pod);
	return *this;
}
// 	Return a normalized quaternion
template<typename T>
uf::Quaternion<T> uf::Quaternion<T>::getNormalized() const {
	return uf::quaternion::normalize(this->m_pod);
}
// 	Quaternion ops
template<typename T>
inline uf::Matrix4t<T> uf::Quaternion<T>::matrix() const {
	return uf::quaternion::matrix(this->m_pod);
}
template<typename T>
inline uf::Quaternion<T>& uf::Quaternion<T>::axisAngle( const Vector3t<T>& axis, T angle ) {
	this->m_pod = uf::quaternion::axisAngle(axis, angle);
	return *this;
}
template<typename T>
inline uf::Quaternion<T>& uf::Quaternion<T>::unitVectors( const Vector3t<T>& u, const Vector3t<T>& v ) {
	this->m_pod = uf::quaternion::unitVectors(u, v);
	return *this;
}

template<typename T>
inline uf::Quaternion<T>& uf::Quaternion<T>::conjugate() {
	return uf::quaternion::conjugate(this->m_pod);
}
template<typename T>
inline uf::Quaternion<T>& uf::Quaternion<T>::inverse() {
	return uf::quaternion::inverse(this->m_pod);
}

template<typename T>
inline uf::Quaternion<T> uf::Quaternion<T>::getConjugate() const {
	return uf::quaternion::conjugate(this->m_pod);
}
template<typename T>
inline uf::Quaternion<T> uf::Quaternion<T>::getInverse() const {
	return uf::quaternion::inverse(this->m_pod);
}
template<typename T>
inline uf::stl::string uf::Quaternion<T>::toString() const {
	return uf::vector::toString(this->m_pod);
}
// 	Overloaded ops
// Accessing via subscripts
template<typename T>
T& uf::Quaternion<T>::operator[](std::size_t i) {
	return this->m_pod[i];
}
template<typename T>
const T& uf::Quaternion<T>::operator[](std::size_t i) const {
	return this->m_pod[i];
}
// Arithmetic
// 	Negation
template<typename T>
inline uf::Quaternion<T> uf::Quaternion<T>::operator-() const {
	return this->negate();
}
// 	Multiplication between two quaternions
template<typename T>
inline uf::Quaternion<T> uf::Quaternion<T>::operator*( const Quaternion<T>& quaternion ) const {
	return this->multiply(quaternion);
}
// 	Multiplication between a quaternion and a vector (rotates vector)
template<typename T>
inline uf::Vector3t<T> uf::Quaternion<T>::operator*( const Vector3t<T>& vector ) const {
	return this->multiply(vector);
}
template<typename T>
inline uf::Vector4t<T> uf::Quaternion<T>::operator*( const Vector4t<T>& vector ) const {
	return this->multiply(vector);
}
// 	Multiplication between a quaternion and a matrix
template<typename T>
inline uf::Matrix4t<T> uf::Quaternion<T>::operator*( const Matrix4t<T>& matrix ) const {
	return this->multiply(matrix);
}
// 	Multiplication set between two quaternions
template<typename T>
inline uf::Quaternion<T>& uf::Quaternion<T>::operator *=( const Quaternion<T>& quaternion ) {
	return this->multiply(quaternion);
}
// 	Equality check between two quaternions (equals)
template<typename T>
inline bool uf::Quaternion<T>::operator==( const Quaternion<T>& quaternion ) const {
	return uf::quaternion::equals(this->m_pod, quaternion.m_pod);
}
// 	Equality check between two quaternions (not equals)
template<typename T>
inline bool uf::Quaternion<T>::operator!=( const Quaternion<T>& quaternion ) const {
	return !uf::quaternion::equals(this->m_pod, quaternion.m_pod);
}
// 	Equality check between two quaternions (less than)
template<typename T>
inline bool uf::Quaternion<T>::operator<( const Quaternion<T>& quaternion ) const {
	return !(uf::quaternion::compareTo(this->m_pod, quaternion.m_pod) < 0);
}
// 	Equality check between two quaternions (less than or equals)
template<typename T>
inline bool uf::Quaternion<T>::operator<=( const Quaternion<T>& quaternion ) const {
	return !(uf::quaternion::compareTo(this->m_pod, quaternion.m_pod) <= 0);
}
// 	Equality check between two quaternions (greater than)
template<typename T>
inline bool uf::Quaternion<T>::operator>( const Quaternion<T>& quaternion ) const {
	return !(uf::quaternion::compareTo(this->m_pod, quaternion.m_pod) > 0);
}
// 	Equality check between two quaternions (greater than or equals)
template<typename T>
inline bool uf::Quaternion<T>::operator>=( const Quaternion<T>& quaternion ) const {
	return !(uf::quaternion::compareTo(this->m_pod, quaternion.m_pod) >= 0);
}