// 	C-tor
template<typename T, std::size_t N> 											// initializes POD to 'def'
uf::Vector<T,N>::Vector() {
}
template<typename T, std::size_t N> 											// initializes POD to 'def'
uf::Vector<T,N>::Vector(T def) {
	for ( std::size_t i = 0; i < N; ++i ) this->m_pod[i] = def;
}
template<typename T, std::size_t N> 											// copies POD altogether
uf::Vector<T,N>::Vector(const typename uf::Vector<T,N>::pod_t& pod) : m_pod(pod) {
}
template<typename T, std::size_t N> 											// copies data into POD from 'components' (typed as C array)
uf::Vector<T,N>::Vector(const T components[N]) {
	this->set(&components[0]);
}
template<typename T, std::size_t N> 											// copies data into POD from 'components' (typed as uf::stl::vector<T>)
uf::Vector<T,N>::Vector(const uf::stl::vector<T>& components) {
	if ( components.size() >= N ) this->set(&components[0]);
}
// 	D-tor
// Unneccesary
// 	POD access
template<typename T, std::size_t N> 											// 	Returns a reference of POD
typename uf::Vector<T,N>::pod_t& uf::Vector<T,N>::data() {
	return this->m_pod;
}
template<typename T, std::size_t N> 											// 	Returns a const-reference of POD
const typename uf::Vector<T,N>::pod_t& uf::Vector<T,N>::data() const {
	return this->m_pod;
}
// 	Alternative POD access
template<typename T, std::size_t N> 											// 	Returns a pointer to the entire array
T* uf::Vector<T,N>::get() {
	return &this->m_pod[0];
}
template<typename T, std::size_t N> 											// 	Returns a const-pointer to the entire array
const T* uf::Vector<T,N>::get() const {
	return &this->m_pod[0];
}
template<typename T, std::size_t N> 											// 	Returns a reference to a single element
T& uf::Vector<T,N>::getComponent( std::size_t i ) {
	return this->m_pod[i];
}
template<typename T, std::size_t N> 											// 	Returns a const-reference to a single element
const T& uf::Vector<T,N>::getComponent( std::size_t i ) const {
	return this->m_pod[i];
}
// 	POD manipulation
template<typename T, std::size_t N> 											// 	Sets the entire array
T* uf::Vector<T,N>::set(const T components[N]) {
	for ( std::size_t i = 0; i < N; ++i ) this->m_pod[i] = components[i];
}
template<typename T, std::size_t N> 											// 	Sets a single element
T& uf::Vector<T,N>::setComponent( std::size_t i, const T& value ) {
	this->m_pod[i] = value;
}
// 	Validation
template<typename T, std::size_t N> 											// 	Checks if all components are valid (non NaN, inf, etc.)
bool uf::Vector<T,N>::isValid() const {
	for ( std::size_t i = 0; i < N; ++i ) if ( this->m_pod[i] != this->m_pod[i] ) return false;
	return true;
}
// 	Basic arithmetic
template<typename T, std::size_t N> 											// 	Adds two vectors of same type and size together
inline uf::Vector<T,N>& uf::Vector<T,N>::add( const Vector<T, N>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T, std::size_t N> 											// 	Multiplies this vector by a scalar
inline uf::Vector<T,N>& uf::Vector<T,N>::add( T scalar ) {
	return uf::vector::add( this->m_pod, scalar );
}
template<typename T, std::size_t N> 											// 	Subtracts two vectors of same type and size together
inline uf::Vector<T,N>& uf::Vector<T,N>::subtract( const Vector<T, N>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T, std::size_t N> 											// 	Multiplies this vector by a scalar
inline uf::Vector<T,N>& uf::Vector<T,N>::subtract( T scalar ) {
	return uf::vector::subtract( this->m_pod, scalar );
}
template<typename T, std::size_t N> 											// 	Multiplies two vectors of same type and size together
inline uf::Vector<T,N>& uf::Vector<T,N>::multiply( const Vector<T, N>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T, std::size_t N> 											// 	Multiplies this vector by a scalar
inline uf::Vector<T,N>& uf::Vector<T,N>::multiply( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T, std::size_t N> 											// 	Divides two vectors of same type and size together
inline uf::Vector<T,N>& uf::Vector<T,N>::divide( const Vector<T, N>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T, std::size_t N> 											// 	Divides this vector by a scalar
inline uf::Vector<T,N>& uf::Vector<T,N>::divide( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T, std::size_t N> 											// 	Compute the sum of all components 
inline T uf::Vector<T,N>::sum() const {
	return uf::vector::sum( this->m_pod );
}
template<typename T, std::size_t N> 											// 	Compute the product of all components 
inline T uf::Vector<T,N>::product() const {
	return uf::vector::product( this->m_pod );
}
template<typename T, std::size_t N> 											// 	Flip sign of all components
inline uf::Vector<T,N>& uf::Vector<T,N>::negate() {
	return uf::vector::negate( this->m_pod );
}
// 	Complex arithmetic
template<typename T, std::size_t N> 											// 	Compute the dot product between two vectors
inline T uf::Vector<T,N>::dot( const Vector<T,N> right ) const {
	return uf::vector::dot( this->m_pod, right );
}
template<typename T, std::size_t N> 											// 	Compute the angle between two vectors
inline float uf::Vector<T,N>::angle( const Vector<T,N>& b ) const {
	return uf::vector::angle( this->m_pod, b );
}

template<typename T, std::size_t N> 											// 	Linearly interpolate between two vectors
inline uf::Vector<T,N> uf::Vector<T,N>::lerp( const Vector<T,N> to, double delta ) const {
	return uf::vector::lerp( this->m_pod, to, delta );
}
template<typename T, std::size_t N> 											// 	Spherically interpolate between two vectors
inline uf::Vector<T,N> uf::Vector<T,N>::slerp( const Vector<T,N> to, double delta ) const {
	return uf::vector::slerp( this->m_pod, to, delta );
}

template<typename T, std::size_t N> 											// 	Compute the distance between two vectors (doesn't sqrt)
inline T uf::Vector<T,N>::distanceSquared( const Vector<T,N> b ) const {
	return uf::vector::distanceSquared( this->m_pod, b );
}
template<typename T, std::size_t N> 											// 	Compute the distance between two vectors
inline T uf::Vector<T,N>::distance( const Vector<T,N> b ) const {
	return uf::vector::distance( this->m_pod, b );
}
template<typename T, std::size_t N> 											// 	Gets the magnitude of the vector
inline T uf::Vector<T,N>::magnitude() const {
	return uf::vector::magnitude( this->m_pod );
}
template<typename T, std::size_t N> 											// 	Compute the norm of the vector
inline T uf::Vector<T,N>::norm() const {
	return uf::vector::norm( this->m_pod );
}

template<typename T, std::size_t N> 											// 	Normalizes a vector
inline uf::Vector<T,N>& uf::Vector<T,N>::normalize() {
	return uf::vector::normalize( this->m_pod );
}
template<typename T, std::size_t N> 											// 	Return a normalized vector
uf::Vector<T,N> uf::Vector<T,N>::getNormalized() const {
	return uf::vector::normalize( this->m_pod );
}
template<typename T, std::size_t N> 											// 	Return a string
uf::stl::string uf::Vector<T,N>::toString() const {
	return uf::vector::toString( this->m_pod );
}
// Overloaded ops
template<typename T, std::size_t N> 
T& uf::Vector<T,N>::operator[](std::size_t i) {
	return this->m_pod[i];
}
template<typename T, std::size_t N> 
const T& uf::Vector<T,N>::operator[](std::size_t i) const {
	return this->m_pod[i];
}
// Arithmetic
template<typename T, std::size_t N>  												// 	Negation
inline uf::Vector<T,N> uf::Vector<T,N>::operator-() const {
	return uf::vector::negate( this->m_pod );
}			
template<typename T, std::size_t N>  												// 	Addition between two vectors
inline uf::Vector<T,N> uf::Vector<T,N>::operator+( const uf::Vector<T,N>& vector ) const {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Subtraction between two vectors
inline uf::Vector<T,N> uf::Vector<T,N>::operator-( const uf::Vector<T,N>& vector ) const {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Multiplication between two vectors
inline uf::Vector<T,N> uf::Vector<T,N>::operator*( const uf::Vector<T,N>& vector ) const {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Division between two vectors
inline uf::Vector<T,N> uf::Vector<T,N>::operator/( const uf::Vector<T,N>& vector ) const {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Multiplication with scalar
inline uf::Vector<T,N> uf::Vector<T,N>::operator+( T scalar ) const {
	return uf::vector::add( this->m_pod, scalar );
}
template<typename T, std::size_t N>  												// 	Multiplication with scalar
inline uf::Vector<T,N> uf::Vector<T,N>::operator-( T scalar ) const {
	return uf::vector::subtract( this->m_pod, scalar );
}
template<typename T, std::size_t N>  												// 	Multiplication with scalar
inline uf::Vector<T,N> uf::Vector<T,N>::operator*( T scalar ) const {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T, std::size_t N>  												// 	Division with scalar
inline uf::Vector<T,N> uf::Vector<T,N>::operator/( T scalar ) const {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T, std::size_t N>  												// 	Addition set between two vectors
inline uf::Vector<T,N>& uf::Vector<T,N>::operator +=( const uf::Vector<T,N>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Subtraction set between two vectors
inline uf::Vector<T,N>& uf::Vector<T,N>::operator -=( const uf::Vector<T,N>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Multiplication set between two vectors
inline uf::Vector<T,N>& uf::Vector<T,N>::operator *=( const uf::Vector<T,N>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Division set between two vectors
inline uf::Vector<T,N>& uf::Vector<T,N>::operator /=( const uf::Vector<T,N>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Multiplication set with scalar
inline uf::Vector<T,N>& uf::Vector<T,N>::operator +=( T scalar ) {
	return uf::vector::add( this->m_pod, scalar );
}
template<typename T, std::size_t N>  												// 	Multiplication set with scalar
inline uf::Vector<T,N>& uf::Vector<T,N>::operator -=( T scalar ) {
	return uf::vector::subtract( this->m_pod, scalar );
}
template<typename T, std::size_t N>  												// 	Multiplication set with scalar
inline uf::Vector<T,N>& uf::Vector<T,N>::operator *=( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T, std::size_t N>  												// 	Division set with scalar
inline uf::Vector<T,N>& uf::Vector<T,N>::operator /=( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T, std::size_t N>  												// 	Equality check between two vectors (equals)
inline bool uf::Vector<T,N>::operator==( const uf::Vector<T,N>& vector ) const {
	return uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Equality check between two vectors (not equals)
inline bool uf::Vector<T,N>::operator!=( const uf::Vector<T,N>& vector ) const {
	return !uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T, std::size_t N>  												// 	Equality check between two vectors (less than)
inline bool uf::Vector<T,N>::operator<( const uf::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) < 0;
}
template<typename T, std::size_t N>  												// 	Equality check between two vectors (less than or equals)
inline bool uf::Vector<T,N>::operator<=( const uf::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) <= 0;
}
template<typename T, std::size_t N>  												// 	Equality check between two vectors (greater than)
inline bool uf::Vector<T,N>::operator>( const uf::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) > 0;
}
template<typename T, std::size_t N>  												// 	Equality check between two vectors (greater than or equals)
inline bool uf::Vector<T,N>::operator>=( const uf::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) >= 0;
}