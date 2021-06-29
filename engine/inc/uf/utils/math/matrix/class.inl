// C-tor
// initializes POD to 'def'
template<typename T, size_t R, size_t C>
uf::Matrix<T,R,C>::Matrix() {
	this->m_pod = uf::matrix::identityi<R,C>();
}
// copies POD altogether
template<typename T, size_t R, size_t C>
uf::Matrix<T,R,C>::Matrix(const Matrix<T,R,C>::pod_t& pod ) :
	m_pod(pod)
{
}
// copies data into POD from 'components' (typed as C array)
template<typename T, size_t R, size_t C>
uf::Matrix<T,R,C>::Matrix(const T components[R][C] ) {
	this->set(components);
}
template<typename T, size_t R, size_t C>
uf::Matrix<T,R,C>::Matrix(const T components[R*C] ) {
	this->set(components);
}
// copies data into POD from 'components' (typed as std::matrix<T>)
template<typename T, size_t R, size_t C>
uf::Matrix<T,R,C>::Matrix(const uf::stl::vector<T>& components ) {
	this->m_pod = uf::matrix::initialize( components );
}
// 	D-tor
// Unneccesary
// 	POD access
// 	Returns a reference of POD
template<typename T, size_t R, size_t C>
typename uf::Matrix<T,R,C>::pod_t& uf::Matrix<T,R,C>::data() {
	return this->m_pod;
}
// 	Returns a const-reference of POD
template<typename T, size_t R, size_t C>
const typename uf::Matrix<T,R,C>::pod_t& uf::Matrix<T,R,C>::data() const {
	return this->m_pod;
}
// 	Returns a const-reference of POD
template<typename T, size_t R, size_t C>
template<typename Q>
typename uf::Matrix<Q,R,C>::pod_t uf::Matrix<T,R,C>::convert() const {
	typename uf::Matrix<Q,R,C>::pod_t converted;
	for ( uint_fast8_t i = 0; i < R*C; ++i )
		converted.components[i] = (float) this->m_pod.components[i];
	return converted;
}
// 	Alternative POD access
// 	Returns a pointer to the entire array
template<typename T, size_t R, size_t C>
T* uf::Matrix<T,R,C>::get() {
	return (T*) this->m_pod.components;
}
// 	Returns a const-pointer to the entire array
template<typename T, size_t R, size_t C>
const T* uf::Matrix<T,R,C>::get() const {
	return (T*) this->m_pod.components;
}
// 	Returns a reference to a single element
template<typename T, size_t R, size_t C>
T& uf::Matrix<T,R,C>::getComponent( uint_fast8_t i ) {
	return this->m_pod.components[i];
}
// 	Returns a const-reference to a single element
template<typename T, size_t R, size_t C>
const T& uf::Matrix<T,R,C>::getComponent( uint_fast8_t i ) const {
	return this->m_pod.components[i];
}
// 	POD manipulation
// 	Sets the entire array
template<typename T, size_t R, size_t C>
T* uf::Matrix<T,R,C>::set(const T components[R][C] ) {
	for ( uint_fast8_t r = 0; r < R; ++r )
		for ( uint_fast8_t c = 0; c < C; ++c )
			this->m_pod.components[r+c*C] = components[r][c];

	return (T*) this->m_pod.components;
}
template<typename T, size_t R, size_t C>
T* uf::Matrix<T,R,C>::set(const T components[R*C] ) {
//	memcpy( this->m_pod.components, components, sizeof(this->m_pod) );
	for ( uint_fast8_t i = 0; i < R*C; ++i )
		this->m_pod.components[i] = components[i];
/*
	for ( size_t r = 0; r < R; ++r )
		for ( size_t c = 0; c < C; ++c )
			this->m_pod.components[r+c*C] = components[r+c*C];
*/

	return (T*) this->m_pod.components;
}
// 	Sets a single element
template<typename T, size_t R, size_t C>
T& uf::Matrix<T,R,C>::setComponent( uint_fast8_t i, const T& value ) {
	this->m_pod.components[i] = value;
}
// 	Validation
// 	Checks if all components are valid (non NaN, inf, etc.)
template<typename T, size_t R, size_t C>
bool uf::Matrix<T,R,C>::isValid() const {
	T val;
	for ( uint_fast8_t i = 0; i < R * C; ++i )
		if ( (val = this->m_pod.components[i]) != val ) return false;
	return true;
}
// 	Basic arithmetic
// 	Multiplies two matrices of same type and size together
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,C,C> uf::Matrix<T,R,C>::multiply( const Matrix<T,R,C>& matrix ) {
	return uf::matrix::multiply(this->m_pod, matrix.data());
}
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,C,C> uf::Matrix<T,R,C>::multiply( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::multiply(this->m_pod, matrix.data());
}
// 	Flip sign of all components
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,R,C>& uf::Matrix<T,R,C>::translate( const pod::Vector3t<T>& vector ) {
	uf::matrix::translate(this->m_pod, vector);
	return *this;
}
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,R,C>& uf::Matrix<T,R,C>::rotate( const pod::Vector3t<T>& vector ) {
	uf::matrix::rotate(this->m_pod, vector);
	return *this;
}
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,R,C>& uf::Matrix<T,R,C>::scale( const pod::Vector3t<T>& vector ) {
	uf::matrix::scale(this->m_pod, vector);
	return *this;
}
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,R,C>& uf::Matrix<T,R,C>::invert() {
	return uf::matrix::invert(this->m_pod);
}
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,R,C>  uf::Matrix<T,R,C>::inverse() const {
	return uf::matrix::inverse(this->m_pod);
}

template<typename T, size_t R, size_t C>
template<typename U> inline uf::Matrix<T,C,C> uf::Matrix<T,R,C>::multiply( const U& matrix ) const {
	return uf::matrix::multiply(this->m_pod, matrix.data());
}
template<typename T, size_t R, size_t C>
template<typename U> inline uf::Matrix<T,C,C> uf::Matrix<T,R,C>::multiply( const U& matrix ) {
	return uf::matrix::multiply(this->m_pod, matrix.data());
}
template<typename T, size_t R, size_t C>
inline pod::Vector3t<T> uf::Matrix<T,R,C>::multiply( const pod::Vector3t<T>& vector ) const {
	return uf::matrix::multiply(this->m_pod, vector);
}
template<typename T, size_t R, size_t C>
inline pod::Vector4t<T> uf::Matrix<T,R,C>::multiply( const pod::Vector4t<T>& vector ) const {
	return uf::matrix::multiply(this->m_pod, vector);
}
// 	Overloaded ops
// Accessing via subscripts
/*
template<typename T, size_t R, size_t C>
T* uf::Matrix<T,R,C>::operator[](size_t i) {
	return this->m_pod[i];
}
template<typename T, size_t R, size_t C>
const T* uf::Matrix<T,R,C>::operator[](size_t i) const {
	return this->m_pod[i];
}
*/
template<typename T, size_t R, size_t C>
T& uf::Matrix<T,R,C>::operator[](uint_fast8_t i) {
	return this->m_pod[i];
}
template<typename T, size_t R, size_t C>
const T& uf::Matrix<T,R,C>::operator[](uint_fast8_t i) const {
	return this->m_pod[i];
}
// Arithmetic
// 	Negation
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,R,C> uf::Matrix<T,R,C>::operator-() const {
	return this->inverse();
}
// 	Multiplication between two matrices
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,C,C> uf::Matrix<T,R,C>::operator*( const Matrix<T,R,C>& matrix ) const {
	return this->multiply(matrix);
}
// 	Multiplication set between two matrices
template<typename T, size_t R, size_t C>
inline uf::Matrix<T,C,C>& uf::Matrix<T,R,C>::operator *=( const Matrix<T,R,C>& matrix ) {
	return this->multiply(matrix);
}
template<typename T, size_t R, size_t C>
template<typename U> inline uf::Matrix<T,C,C> uf::Matrix<T,R,C>::operator*( const U& matrix ) const {
	return this->multiply(matrix);
}
// 	Equality check between two matrices (equals)
template<typename T, size_t R, size_t C>
inline bool uf::Matrix<T,R,C>::operator==( const Matrix<T,R,C>& matrix ) const {
	return uf::matrix::equals( this->m_pod, matrix.m_pod );
}
// 	Equality check between two matrices (not equals)
template<typename T, size_t R, size_t C>
inline bool uf::Matrix<T,R,C>::operator!=( const Matrix<T,R,C>& matrix ) const {
	return !uf::matrix::equals( this->m_pod, matrix.m_pod );
}