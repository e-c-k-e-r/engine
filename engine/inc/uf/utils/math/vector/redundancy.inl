//
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
inline pod::Vector<T,N> pod::Vector<T,N>::operator+( T scalar ) const {
	return uf::vector::add( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Multiplication with scalar
inline pod::Vector<T,N> pod::Vector<T,N>::operator-( T scalar ) const {
	return uf::vector::subtract( *this, scalar );
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
inline pod::Vector<T,N>& pod::Vector<T,N>::operator +=( T scalar ) {
	return uf::vector::add( *this, scalar );
}
template<typename T, std::size_t N> 												// 	Multiplication set with scalar
inline pod::Vector<T,N>& pod::Vector<T,N>::operator -=( T scalar ) {
	return uf::vector::subtract( *this, scalar );
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
	return uf::vector::compareTo(*this, vector) < 0;
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (less than or equals)
inline bool pod::Vector<T,N>::operator<=( const pod::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(*this, vector) <= 0;
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (greater than)
inline bool pod::Vector<T,N>::operator>( const pod::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(*this, vector) > 0;
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (greater than or equals)
inline bool pod::Vector<T,N>::operator>=( const pod::Vector<T,N>& vector ) const {
	return uf::vector::compareTo(*this, vector) >= 0;
}
template<typename T, std::size_t N> 												// 	Equality check between two vectors (greater than or equals)
inline pod::Vector<T,N>::operator bool() const {
	return !uf::vector::equals(*this, pod::Vector<T,N>{});
}

template<typename T, size_t N>
template<typename U, size_t M>
pod::Vector<T,N>::operator pod::Vector<U,M>() {
	return uf::vector::cast<U,M>(*this);
}
template<typename T, size_t N>
template<typename U, size_t M>
pod::Vector<T,N>& pod::Vector<T,N>::operator=( const pod::Vector<U,M>& vector ) {
	return *this = uf::vector::cast<T,N>(vector);
}
//
template<typename T>
T& pod::Vector<T,1>::operator[](std::size_t i) {
	if ( i >= this->size ) i = this->size - 1;
	return (&this->x)[i];
}
template<typename T>
const T& pod::Vector<T,1>::operator[](std::size_t i) const {
	if ( i >= this->size ) i = this->size - 1;
	return (&this->x)[i];
}
// Arithmetic
template<typename T> 												// 	Negation
inline pod::Vector<T,1> pod::Vector<T,1>::operator()() const {
	return uf::vector::create<T>(0);
}			
template<typename T> 												// 	Negation
inline pod::Vector<T,1> pod::Vector<T,1>::operator-() const {
	return uf::vector::negate( *this );
}			
template<typename T> 												// 	Addition between two vectors
inline pod::Vector<T,1> pod::Vector<T,1>::operator+( const pod::Vector<T,1>& vector ) const {
	return uf::vector::add( *this, vector );
}
template<typename T> 												// 	Subtraction between two vectors
inline pod::Vector<T,1> pod::Vector<T,1>::operator-( const pod::Vector<T,1>& vector ) const {
	return uf::vector::subtract( *this, vector );
}
template<typename T> 												// 	Multiplication between two vectors
inline pod::Vector<T,1> pod::Vector<T,1>::operator*( const pod::Vector<T,1>& vector ) const {
	return uf::vector::multiply( *this, vector );
}
template<typename T> 												// 	Division between two vectors
inline pod::Vector<T,1> pod::Vector<T,1>::operator/( const pod::Vector<T,1>& vector ) const {
	return uf::vector::divide( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,1> pod::Vector<T,1>::operator+( T scalar ) const {
	return uf::vector::add( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,1> pod::Vector<T,1>::operator-( T scalar ) const {
	return uf::vector::subtract( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,1> pod::Vector<T,1>::operator*( T scalar ) const {
	return uf::vector::multiply( *this, scalar );
}
template<typename T> 												// 	Division with scalar
inline pod::Vector<T,1> pod::Vector<T,1>::operator/( T scalar ) const {
	return uf::vector::divide( *this, scalar );
}
template<typename T> 												// 	Addition set between two vectors
inline pod::Vector<T,1>& pod::Vector<T,1>::operator +=( const pod::Vector<T,1>& vector ) {
	return uf::vector::add( *this, vector );
}
template<typename T> 												// 	Subtraction set between two vectors
inline pod::Vector<T,1>& pod::Vector<T,1>::operator -=( const pod::Vector<T,1>& vector ) {
	return uf::vector::subtract( *this, vector );
}
template<typename T> 												// 	Multiplication set between two vectors
inline pod::Vector<T,1>& pod::Vector<T,1>::operator *=( const pod::Vector<T,1>& vector ) {
	return uf::vector::multiply( *this, vector );
}
template<typename T> 												// 	Division set between two vectors
inline pod::Vector<T,1>& pod::Vector<T,1>::operator /=( const pod::Vector<T,1>& vector ) {
	return uf::vector::divide( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,1>& pod::Vector<T,1>::operator +=( T scalar ) {
	return uf::vector::add( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,1>& pod::Vector<T,1>::operator -=( T scalar ) {
	return uf::vector::subtract( *this, scalar );
}
template<typename T> 												// 	Multiplication set with scalar
inline pod::Vector<T,1>& pod::Vector<T,1>::operator *=( T scalar ) {
	return uf::vector::multiply( *this, scalar );
}
template<typename T> 												// 	Division set with scalar
inline pod::Vector<T,1>& pod::Vector<T,1>::operator /=( T scalar ) {
	return uf::vector::divide( *this, scalar );
}
template<typename T> 												// 	Equality check between two vectors (equals)
inline bool pod::Vector<T,1>::operator==( const pod::Vector<T,1>& vector ) const {
	return uf::vector::equals(*this, vector);
}
template<typename T> 												// 	Equality check between two vectors (not equals)
inline bool pod::Vector<T,1>::operator!=( const pod::Vector<T,1>& vector ) const {
	return !uf::vector::equals(*this, vector);
}
template<typename T> 												// 	Equality check between two vectors (less than)
inline bool pod::Vector<T,1>::operator<( const pod::Vector<T,1>& vector ) const {
	return uf::vector::compareTo(*this, vector) < 0;
}
template<typename T> 												// 	Equality check between two vectors (less than or equals)
inline bool pod::Vector<T,1>::operator<=( const pod::Vector<T,1>& vector ) const {
	return uf::vector::compareTo(*this, vector) <= 0;
}
template<typename T> 												// 	Equality check between two vectors (greater than)
inline bool pod::Vector<T,1>::operator>( const pod::Vector<T,1>& vector ) const {
	return uf::vector::compareTo(*this, vector) > 0;
}
template<typename T> 												// 	Equality check between two vectors (greater than or equals)
inline bool pod::Vector<T,1>::operator>=( const pod::Vector<T,1>& vector ) const {
	return uf::vector::compareTo(*this, vector) >= 0;
}
template<typename T>
inline pod::Vector<T,1>::operator bool() const {
	return !uf::vector::equals(*this, pod::Vector<T,1>{});
}
template<typename T>
template<typename U, std::size_t M>
pod::Vector<T,1>::operator pod::Vector<U,M>() {
	return uf::vector::cast<U,M>(*this);
}
template<typename T>
template<typename U, std::size_t M>
pod::Vector<T,1>& pod::Vector<T,1>::operator=( const pod::Vector<U,M>& vector ) {
	return *this = uf::vector::cast<T,1>(vector);
}
//
template<typename T>
T& pod::Vector<T,2>::operator[](std::size_t i) {
	if ( i >= this->size ) i = this->size - 1;
	return (&this->x)[i];
}
template<typename T>
const T& pod::Vector<T,2>::operator[](std::size_t i) const {
	if ( i >= this->size ) i = this->size - 1;
	return (&this->x)[i];
}
// Arithmetic
template<typename T> 												// 	Negation
inline pod::Vector<T,2> pod::Vector<T,2>::operator()() const {
	return uf::vector::create<T>(0,0);
}			
template<typename T> 												// 	Negation
inline pod::Vector<T,2> pod::Vector<T,2>::operator-() const {
	return uf::vector::negate( *this );
}			
template<typename T> 												// 	Addition between two vectors
inline pod::Vector<T,2> pod::Vector<T,2>::operator+( const pod::Vector<T,2>& vector ) const {
	return uf::vector::add( *this, vector );
}
template<typename T> 												// 	Subtraction between two vectors
inline pod::Vector<T,2> pod::Vector<T,2>::operator-( const pod::Vector<T,2>& vector ) const {
	return uf::vector::subtract( *this, vector );
}
template<typename T> 												// 	Multiplication between two vectors
inline pod::Vector<T,2> pod::Vector<T,2>::operator*( const pod::Vector<T,2>& vector ) const {
	return uf::vector::multiply( *this, vector );
}
template<typename T> 												// 	Division between two vectors
inline pod::Vector<T,2> pod::Vector<T,2>::operator/( const pod::Vector<T,2>& vector ) const {
	return uf::vector::divide( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,2> pod::Vector<T,2>::operator+( T scalar ) const {
	return uf::vector::add( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,2> pod::Vector<T,2>::operator-( T scalar ) const {
	return uf::vector::subtract( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,2> pod::Vector<T,2>::operator*( T scalar ) const {
	return uf::vector::multiply( *this, scalar );
}
template<typename T> 												// 	Division with scalar
inline pod::Vector<T,2> pod::Vector<T,2>::operator/( T scalar ) const {
	return uf::vector::divide( *this, scalar );
}
template<typename T> 												// 	Addition set between two vectors
inline pod::Vector<T,2>& pod::Vector<T,2>::operator +=( const pod::Vector<T,2>& vector ) {
	return uf::vector::add( *this, vector );
}
template<typename T> 												// 	Subtraction set between two vectors
inline pod::Vector<T,2>& pod::Vector<T,2>::operator -=( const pod::Vector<T,2>& vector ) {
	return uf::vector::subtract( *this, vector );
}
template<typename T> 												// 	Multiplication set between two vectors
inline pod::Vector<T,2>& pod::Vector<T,2>::operator *=( const pod::Vector<T,2>& vector ) {
	return uf::vector::multiply( *this, vector );
}
template<typename T> 												// 	Division set between two vectors
inline pod::Vector<T,2>& pod::Vector<T,2>::operator /=( const pod::Vector<T,2>& vector ) {
	return uf::vector::divide( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,2>& pod::Vector<T,2>::operator+=( T scalar ) {
	return uf::vector::add( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,2>& pod::Vector<T,2>::operator-=( T scalar ) {
	return uf::vector::subtract( *this, scalar );
}
template<typename T> 												// 	Multiplication set with scalar
inline pod::Vector<T,2>& pod::Vector<T,2>::operator *=( T scalar ) {
	return uf::vector::multiply( *this, scalar );
}
template<typename T> 												// 	Division set with scalar
inline pod::Vector<T,2>& pod::Vector<T,2>::operator /=( T scalar ) {
	return uf::vector::divide( *this, scalar );
}
template<typename T> 												// 	Equality check between two vectors (equals)
inline bool pod::Vector<T,2>::operator==( const pod::Vector<T,2>& vector ) const {
	return uf::vector::equals(*this, vector);
}
template<typename T> 												// 	Equality check between two vectors (not equals)
inline bool pod::Vector<T,2>::operator!=( const pod::Vector<T,2>& vector ) const {
	return !uf::vector::equals(*this, vector);
}
template<typename T> 												// 	Equality check between two vectors (less than)
inline bool pod::Vector<T,2>::operator<( const pod::Vector<T,2>& vector ) const {
	return uf::vector::compareTo(*this, vector) < 0;
}
template<typename T> 												// 	Equality check between two vectors (less than or equals)
inline bool pod::Vector<T,2>::operator<=( const pod::Vector<T,2>& vector ) const {
	return uf::vector::compareTo(*this, vector) <= 0;
}
template<typename T> 												// 	Equality check between two vectors (greater than)
inline bool pod::Vector<T,2>::operator>( const pod::Vector<T,2>& vector ) const {
	return uf::vector::compareTo(*this, vector) > 0;
}
template<typename T> 												// 	Equality check between two vectors (greater than or equals)
inline bool pod::Vector<T,2>::operator>=( const pod::Vector<T,2>& vector ) const {
	return uf::vector::compareTo(*this, vector) >= 0;
}
template<typename T>
inline pod::Vector<T,2>::operator bool() const {
	return !uf::vector::equals(*this, pod::Vector<T,2>{});
}
template<typename T>
template<typename U, std::size_t M>
pod::Vector<T,2>::operator pod::Vector<U,M>() {
	return uf::vector::cast<U,M>(*this);
}
template<typename T>
template<typename U, std::size_t M>
pod::Vector<T,2>& pod::Vector<T,2>::operator=( const pod::Vector<U,M>& vector ) {
	return *this = uf::vector::cast<T,2>(vector);
}
//
template<typename T>
T& pod::Vector<T,3>::operator[](std::size_t i) {
	if ( i >= this->size ) i = this->size - 1;
	return (&this->x)[i];
}
template<typename T>
const T& pod::Vector<T,3>::operator[](std::size_t i) const {
	if ( i >= this->size ) i = this->size - 1;
	return (&this->x)[i];
}
// Arithmetic
template<typename T> 												// 	Negation
inline pod::Vector<T,3> pod::Vector<T,3>::operator()() const {
	return uf::vector::create<T>(0,0,0);
}			
template<typename T> 												// 	Negation
inline pod::Vector<T,3> pod::Vector<T,3>::operator-() const {
	return uf::vector::negate( *this );
}			
template<typename T> 												// 	Addition between two vectors
inline pod::Vector<T,3> pod::Vector<T,3>::operator+( const pod::Vector<T,3>& vector ) const {
	return uf::vector::add( *this, vector );
}
template<typename T> 												// 	Subtraction between two vectors
inline pod::Vector<T,3> pod::Vector<T,3>::operator-( const pod::Vector<T,3>& vector ) const {
	return uf::vector::subtract( *this, vector );
}
template<typename T> 												// 	Multiplication between two vectors
inline pod::Vector<T,3> pod::Vector<T,3>::operator*( const pod::Vector<T,3>& vector ) const {
	return uf::vector::multiply( *this, vector );
}
template<typename T> 												// 	Division between two vectors
inline pod::Vector<T,3> pod::Vector<T,3>::operator/( const pod::Vector<T,3>& vector ) const {
	return uf::vector::divide( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,3> pod::Vector<T,3>::operator+( T scalar ) const {
	return uf::vector::add( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,3> pod::Vector<T,3>::operator-( T scalar ) const {
	return uf::vector::subtract( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,3> pod::Vector<T,3>::operator*( T scalar ) const {
	return uf::vector::multiply( *this, scalar );
}
template<typename T> 												// 	Division with scalar
inline pod::Vector<T,3> pod::Vector<T,3>::operator/( T scalar ) const {
	return uf::vector::divide( *this, scalar );
}
template<typename T> 												// 	Addition set between two vectors
inline pod::Vector<T,3>& pod::Vector<T,3>::operator +=( const pod::Vector<T,3>& vector ) {
	return uf::vector::add( *this, vector );
}
template<typename T> 												// 	Subtraction set between two vectors
inline pod::Vector<T,3>& pod::Vector<T,3>::operator -=( const pod::Vector<T,3>& vector ) {
	return uf::vector::subtract( *this, vector );
}
template<typename T> 												// 	Multiplication set between two vectors
inline pod::Vector<T,3>& pod::Vector<T,3>::operator *=( const pod::Vector<T,3>& vector ) {
	return uf::vector::multiply( *this, vector );
}
template<typename T> 												// 	Division set between two vectors
inline pod::Vector<T,3>& pod::Vector<T,3>::operator /=( const pod::Vector<T,3>& vector ) {
	return uf::vector::divide( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,3>& pod::Vector<T,3>::operator +=( T scalar ) {
	return uf::vector::add( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,3>& pod::Vector<T,3>::operator -=( T scalar ) {
	return uf::vector::subtract( *this, scalar );
}
template<typename T> 												// 	Multiplication set with scalar
inline pod::Vector<T,3>& pod::Vector<T,3>::operator *=( T scalar ) {
	return uf::vector::multiply( *this, scalar );
}
template<typename T> 												// 	Division set with scalar
inline pod::Vector<T,3>& pod::Vector<T,3>::operator /=( T scalar ) {
	return uf::vector::divide( *this, scalar );
}
template<typename T> 												// 	Equality check between two vectors (equals)
inline bool pod::Vector<T,3>::operator==( const pod::Vector<T,3>& vector ) const {
	return uf::vector::equals(*this, vector);
}
template<typename T> 												// 	Equality check between two vectors (not equals)
inline bool pod::Vector<T,3>::operator!=( const pod::Vector<T,3>& vector ) const {
	return !uf::vector::equals(*this, vector);
}
template<typename T> 												// 	Equality check between two vectors (less than)
inline bool pod::Vector<T,3>::operator<( const pod::Vector<T,3>& vector ) const {
	return uf::vector::compareTo(*this, vector) < 0;
}
template<typename T> 												// 	Equality check between two vectors (less than or equals)
inline bool pod::Vector<T,3>::operator<=( const pod::Vector<T,3>& vector ) const {
	return uf::vector::compareTo(*this, vector) <= 0;
}
template<typename T> 												// 	Equality check between two vectors (greater than)
inline bool pod::Vector<T,3>::operator>( const pod::Vector<T,3>& vector ) const {
	return uf::vector::compareTo(*this, vector) > 0;
}
template<typename T> 												// 	Equality check between two vectors (greater than or equals)
inline bool pod::Vector<T,3>::operator>=( const pod::Vector<T,3>& vector ) const {
	return uf::vector::compareTo(*this, vector) >= 0;
}
template<typename T>
inline pod::Vector<T,3>::operator bool() const {
	return !uf::vector::equals(*this, pod::Vector<T,3>{});
}
template<typename T>
template<typename U, std::size_t M>
pod::Vector<T,3>::operator pod::Vector<U,M>() {
	return uf::vector::cast<U,M>(*this);
/*
	pod::Vector<U,M> vector = {};
	for ( size_t i = 0; i < 3 && i < M; ++i ) {
		vector[i] = (*this)[i];
	}
	return vector;
*/
}
template<typename T>
template<typename U, std::size_t M>
pod::Vector<T,3>& pod::Vector<T,3>::operator=( const pod::Vector<U,M>& vector ) {
	return *this = uf::vector::cast<T,3>(vector);
/*
	for ( size_t i = 0; i < 3 && i < M; ++i ) {
		(*this)[i] = vector[i];
	}
	return *this;
*/
}
//
template<typename T>
T& pod::Vector<T,4>::operator[](std::size_t i) {
	if ( i >= this->size ) i = this->size - 1;
	return (&this->x)[i];
}
template<typename T>
const T& pod::Vector<T,4>::operator[](std::size_t i) const {
	if ( i >= this->size ) i = this->size - 1;
	return (&this->x)[i];
}
// Arithmetic
template<typename T> 												// 	Negation
inline pod::Vector<T,4> pod::Vector<T,4>::operator()() const {
	return uf::vector::create<T>(0,0,0,1);
}			
template<typename T> 												// 	Negation
inline pod::Vector<T,4> pod::Vector<T,4>::operator-() const {
	return uf::vector::negate( *this );
}			
template<typename T> 												// 	Addition between two vectors
inline pod::Vector<T,4> pod::Vector<T,4>::operator+( const pod::Vector<T,4>& vector ) const {
	return uf::vector::add( *this, vector );
}
template<typename T> 												// 	Subtraction between two vectors
inline pod::Vector<T,4> pod::Vector<T,4>::operator-( const pod::Vector<T,4>& vector ) const {
	return uf::vector::subtract( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,4> pod::Vector<T,4>::operator+( T scalar ) const {
	return uf::vector::add( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,4> pod::Vector<T,4>::operator-( T scalar ) const {
	return uf::vector::subtract( *this, scalar );
}
template<typename T> 												// 	Multiplication between two vectors
inline pod::Vector<T,4> pod::Vector<T,4>::operator*( const pod::Vector<T,4>& vector ) const {
	return uf::vector::multiply( *this, vector );
}
template<typename T> 												// 	Division between two vectors
inline pod::Vector<T,4> pod::Vector<T,4>::operator/( const pod::Vector<T,4>& vector ) const {
	return uf::vector::divide( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,4> pod::Vector<T,4>::operator*( T scalar ) const {
	return uf::vector::multiply( *this, scalar );
}
template<typename T> 												// 	Division with scalar
inline pod::Vector<T,4> pod::Vector<T,4>::operator/( T scalar ) const {
	return uf::vector::divide( *this, scalar );
}
template<typename T> 												// 	Addition set between two vectors
inline pod::Vector<T,4>& pod::Vector<T,4>::operator +=( const pod::Vector<T,4>& vector ) {
	return uf::vector::add( *this, vector );
}
template<typename T> 												// 	Subtraction set between two vectors
inline pod::Vector<T,4>& pod::Vector<T,4>::operator -=( const pod::Vector<T,4>& vector ) {
	return uf::vector::subtract( *this, vector );
}
template<typename T> 												// 	Multiplication set between two vectors
inline pod::Vector<T,4>& pod::Vector<T,4>::operator *=( const pod::Vector<T,4>& vector ) {
	return uf::vector::multiply( *this, vector );
}
template<typename T> 												// 	Division set between two vectors
inline pod::Vector<T,4>& pod::Vector<T,4>::operator /=( const pod::Vector<T,4>& vector ) {
	return uf::vector::divide( *this, vector );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,4>& pod::Vector<T,4>::operator+=( T scalar ) {
	return uf::vector::add( *this, scalar );
}
template<typename T> 												// 	Multiplication with scalar
inline pod::Vector<T,4>& pod::Vector<T,4>::operator-=( T scalar ) {
	return uf::vector::subtract( *this, scalar );
}
template<typename T> 												// 	Multiplication set with scalar
inline pod::Vector<T,4>& pod::Vector<T,4>::operator *=( T scalar ) {
	return uf::vector::multiply( *this, scalar );
}
template<typename T> 												// 	Division set with scalar
inline pod::Vector<T,4>& pod::Vector<T,4>::operator /=( T scalar ) {
	return uf::vector::divide( *this, scalar );
}
template<typename T> 												// 	Equality check between two vectors (equals)
inline bool pod::Vector<T,4>::operator==( const pod::Vector<T,4>& vector ) const {
	return uf::vector::equals(*this, vector);
}
template<typename T> 												// 	Equality check between two vectors (not equals)
inline bool pod::Vector<T,4>::operator!=( const pod::Vector<T,4>& vector ) const {
	return !uf::vector::equals(*this, vector);
}
template<typename T> 												// 	Equality check between two vectors (less than)
inline bool pod::Vector<T,4>::operator<( const pod::Vector<T,4>& vector ) const {
	return uf::vector::compareTo(*this, vector) < 0;
}
template<typename T> 												// 	Equality check between two vectors (less than or equals)
inline bool pod::Vector<T,4>::operator<=( const pod::Vector<T,4>& vector ) const {
	return uf::vector::compareTo(*this, vector) <= 0;
}
template<typename T> 												// 	Equality check between two vectors (greater than)
inline bool pod::Vector<T,4>::operator>( const pod::Vector<T,4>& vector ) const {
	return uf::vector::compareTo(*this, vector) > 0;
}
template<typename T> 												// 	Equality check between two vectors (greater than or equals)
inline bool pod::Vector<T,4>::operator>=( const pod::Vector<T,4>& vector ) const {
	return uf::vector::compareTo(*this, vector) >= 0;
}
template<typename T> 
inline pod::Vector<T,4>::operator bool() const {
	return !uf::vector::equals(*this, pod::Vector<T,4>{});
}
template<typename T>
template<typename U, std::size_t M>
pod::Vector<T,4>::operator pod::Vector<U,M>() {
	return uf::vector::cast<U,M>(*this);
}
template<typename T>
template<typename U, std::size_t M>
pod::Vector<T,4>& pod::Vector<T,4>::operator=( const pod::Vector<U,M>& vector ) {
	return *this = uf::vector::cast<T,4>(vector);
}
//
namespace uf {
	template<typename T>
	struct /*UF_API*/ Vector<T,1> {
	public:
	// 	Easily access POD's type
		typedef pod::Vector<T,1> pod_t;
	// 	Replicate POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 1;
	protected:
	// 	POD storage
		Vector<T,1>::pod_t m_pod;
	public:
		T& x = m_pod.x;
	public:
	// 	C-tor
		Vector(); 																		// initializes POD to 'def'
		Vector(T def); 																	// initializes POD to 'def'
		Vector(T x, T y); 																// initializes POD to 'def'
		Vector(const Vector<T,1>::pod_t& pod); 											// copies POD altogether
		Vector(const T components[1]); 													// copies data into POD from 'components' (typed as C array)
		Vector(const uf::stl::vector<T>& components); 										// copies data into POD from 'components' (typed as uf::stl::vector<T>)
	// 	D-tor
		// Unneccesary
	// 	POD access
		Vector<T,1>::pod_t& data(); 													// 	Returns a reference of POD
		const Vector<T,1>::pod_t& data() const; 										// 	Returns a const-reference of POD
	// 	Alternative POD access
		T* get();																		// 	Returns a pointer to the entire array
		const T* get() const; 															// 	Returns a const-pointer to the entire array
		T& getComponent( std::size_t i );												// 	Returns a reference to a single element
		const T& getComponent( std::size_t i ) const; 									// 	Returns a const-reference to a single element
	// 	POD manipulation
		T* set(const T components[1]);													// 	Sets the entire array
		T& setComponent( std::size_t i, const T& value );								// 	Sets a single element
	// 	Validation
		bool isValid() const; 															// 	Checks if all components are valid (non NaN, inf, etc.)
	// 	Basic arithmetic
		inline uf::Vector<T,1>& add( const Vector<T, 1>& vector );						// 	Adds two vectors of same type and size together
		inline uf::Vector<T,1>& subtract( const Vector<T, 1>& vector );					// 	Subtracts two vectors of same type and size together
		inline uf::Vector<T,1>& multiply( const Vector<T, 1>& vector );					// 	Multiplies two vectors of same type and size together
		inline uf::Vector<T,1>& multiply( T scalar );									// 	Multiplies this vector by a scalar
		inline uf::Vector<T,1>& divide( const Vector<T, 1>& vector );					// 	Divides two vectors of same type and size together
		inline uf::Vector<T,1>& divide( T scalar );										// 	Divides this vector by a scalar
		inline T sum() const;															// 	Compute the sum of all components 
		inline T product() const;														// 	Compute the product of all components 
		inline uf::Vector<T,1>& negate();												// 	Flip sign of all components
	// 	Complex arithmetic
		inline T dot( const Vector<T,1> right ) const; 									// 	Compute the dot product between two vectors
		inline pod::Angle angle( const Vector<T,1>& b ) const; 							// 	Compute the angle between two vectors
		
		inline uf::Vector<T,1> lerp( const Vector<T,1> to, double delta ) const; 		// 	Linearly interpolate between two vectors
		inline uf::Vector<T,1> slerp( const Vector<T,1> to, double delta ) const; 		// 	Spherically interpolate between two vectors
		
		inline T distanceSquared( const Vector<T,1> b ) const; 							// 	Compute the distance between two vectors (doesn't sqrt)
		inline T distance( const Vector<T,1> b ) const; 								// 	Compute the distance between two vectors
		inline T magnitude() const; 													// 	Gets the magnitude of the vector
		inline T norm() const; 															// 	Compute the norm of the vector
		
		inline uf::Vector<T,1>& normalize(); 											// 	Normalizes a vector
		uf::Vector<T,1> getNormalized() const; 											// 	Return a normalized vector
		inline uf::stl::string toString() const;
	// 	Overloaded ops
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,1> operator-() const; 								// 	Negation
		inline Vector<T,1> operator+( const Vector<T,1>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,1> operator-( const Vector<T,1>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,1> operator*( const Vector<T,1>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,1> operator/( const Vector<T,1>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,1> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,1> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,1>& operator +=( const Vector<T,1>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,1>& operator -=( const Vector<T,1>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,1>& operator *=( const Vector<T,1>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,1>& operator /=( const Vector<T,1>& vector ); 		// 	Division set between two vectors
		inline Vector<T,1>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,1>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,1>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		
		inline operator pod_t&() { return this->m_pod; }
		inline operator const pod_t&() const { return this->m_pod; }
	};
	template<typename T>
	struct /*UF_API*/ Vector<T,2> {
	public:
	// 	Easily access POD's type
		typedef pod::Vector<T,2> pod_t;
	// 	Replicate POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 2;
	protected:
	// 	POD storage
		Vector<T,2>::pod_t m_pod;
	public:
		T& x = m_pod.x;
		T& y = m_pod.y;
	public:
	// 	C-tor
		Vector(); 																		// initializes POD to 'def'
		Vector(T def); 																	// initializes POD to 'def'
		Vector(T x, T y); 																// initializes POD to 'def'
		Vector(const Vector<T,2>::pod_t& pod); 											// copies POD altogether
		Vector(const T components[2]); 													// copies data into POD from 'components' (typed as C array)
		Vector(const uf::stl::vector<T>& components); 										// copies data into POD from 'components' (typed as uf::stl::vector<T>)
	// 	D-tor
		// Unneccesary
	// 	POD access
		Vector<T,2>::pod_t& data(); 													// 	Returns a reference of POD
		const Vector<T,2>::pod_t& data() const; 										// 	Returns a const-reference of POD
	// 	Alternative POD access
		T* get();																		// 	Returns a pointer to the entire array
		const T* get() const; 															// 	Returns a const-pointer to the entire array
		T& getComponent( std::size_t i );												// 	Returns a reference to a single element
		const T& getComponent( std::size_t i ) const; 									// 	Returns a const-reference to a single element
	// 	POD manipulation
		T* set(const T components[2]);													// 	Sets the entire array
		T& setComponent( std::size_t i, const T& value );								// 	Sets a single element
	// 	Validation
		bool isValid() const; 															// 	Checks if all components are valid (non NaN, inf, etc.)
	// 	Basic arithmetic
		inline uf::Vector<T,2>& add( const Vector<T, 2>& vector );						// 	Adds two vectors of same type and size together
		inline uf::Vector<T,2>& subtract( const Vector<T, 2>& vector );					// 	Subtracts two vectors of same type and size together
		inline uf::Vector<T,2>& multiply( const Vector<T, 2>& vector );					// 	Multiplies two vectors of same type and size together
		inline uf::Vector<T,2>& multiply( T scalar );									// 	Multiplies this vector by a scalar
		inline uf::Vector<T,2>& divide( const Vector<T, 2>& vector );					// 	Divides two vectors of same type and size together
		inline uf::Vector<T,2>& divide( T scalar );										// 	Divides this vector by a scalar
		inline T sum() const;															// 	Compute the sum of all components 
		inline T product() const;														// 	Compute the product of all components 
		inline uf::Vector<T,2>& negate();												// 	Flip sign of all components
	// 	Complex arithmetic
		inline T dot( const Vector<T,2> right ) const; 									// 	Compute the dot product between two vectors
		inline pod::Angle angle( const Vector<T,2>& b ) const; 							// 	Compute the angle between two vectors
		
		inline uf::Vector<T,2> lerp( const Vector<T,2> to, double delta ) const; 		// 	Linearly interpolate between two vectors
		inline uf::Vector<T,2> slerp( const Vector<T,2> to, double delta ) const; 		// 	Spherically interpolate between two vectors
		
		inline T distanceSquared( const Vector<T,2> b ) const; 							// 	Compute the distance between two vectors (doesn't sqrt)
		inline T distance( const Vector<T,2> b ) const; 								// 	Compute the distance between two vectors
		inline T magnitude() const; 													// 	Gets the magnitude of the vector
		inline T norm() const; 															// 	Compute the norm of the vector
		
		inline uf::Vector<T,2>& normalize(); 											// 	Normalizes a vector
		uf::Vector<T,2> getNormalized() const; 											// 	Return a normalized vector
		inline uf::stl::string toString() const;
	// 	Overloaded ops
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,2> operator-() const; 								// 	Negation
		inline Vector<T,2> operator+( const Vector<T,2>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,2> operator-( const Vector<T,2>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,2> operator*( const Vector<T,2>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,2> operator/( const Vector<T,2>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,2> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,2> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,2>& operator +=( const Vector<T,2>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,2>& operator -=( const Vector<T,2>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,2>& operator *=( const Vector<T,2>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,2>& operator /=( const Vector<T,2>& vector ); 		// 	Division set between two vectors
		inline Vector<T,2>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,2>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,2>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		
		inline operator pod_t&() { return this->m_pod; }
		inline operator const pod_t&() const { return this->m_pod; }
	};
	template<typename T>
	struct /*UF_API*/ Vector<T,3> {
	public:
	// 	Easily access POD's type
		typedef pod::Vector<T,3> pod_t;
	// 	Replicate POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 3;
	protected:
	// 	POD storage
		Vector<T,3>::pod_t m_pod;
	public:
		T& x = m_pod.x;
		T& y = m_pod.y;
		T& z = m_pod.z;

		T& r = m_pod.x;
		T& g = m_pod.y;
		T& b = m_pod.z;
	public:
	// 	C-tor
		Vector(); 																		// initializes POD to 'def'
		Vector(T def); 																	// initializes POD to 'def'
		Vector(T x, T y, T z); 															// initializes POD to 'def'
		Vector(const Vector<T,3>::pod_t& pod); 											// copies POD altogether
		Vector(const T components[3]); 													// copies data into POD from 'components' (typed as C array)
		Vector(const uf::stl::vector<T>& components); 										// copies data into POD from 'components' (typed as uf::stl::vector<T>)
	// 	D-tor
		// Unneccesary
	// 	POD access
		Vector<T,3>::pod_t& data(); 													// 	Returns a reference of POD
		const Vector<T,3>::pod_t& data() const; 										// 	Returns a const-reference of POD
	// 	Alternative POD access
		T* get();																		// 	Returns a pointer to the entire array
		const T* get() const; 															// 	Returns a const-pointer to the entire array
		T& getComponent( std::size_t i );												// 	Returns a reference to a single element
		const T& getComponent( std::size_t i ) const; 									// 	Returns a const-reference to a single element
	// 	POD manipulation
		T* set(const T components[3]);													// 	Sets the entire array
		T& setComponent( std::size_t i, const T& value );								// 	Sets a single element
	// 	Validation
		bool isValid() const; 															// 	Checks if all components are valid (non NaN, inf, etc.)
	// 	Basic arithmetic
		inline uf::Vector<T,3>& add( const Vector<T, 3>& vector );						// 	Adds two vectors of same type and size together
		inline uf::Vector<T,3>& subtract( const Vector<T, 3>& vector );					// 	Subtracts two vectors of same type and size together
		inline uf::Vector<T,3>& multiply( const Vector<T, 3>& vector );					// 	Multiplies two vectors of same type and size together
		inline uf::Vector<T,3>& multiply( T scalar );									// 	Multiplies this vector by a scalar
		inline uf::Vector<T,3>& divide( const Vector<T, 3>& vector );					// 	Divides two vectors of same type and size together
		inline uf::Vector<T,3>& divide( T scalar );										// 	Divides this vector by a scalar
		inline T sum() const;															// 	Compute the sum of all components 
		inline T product() const;														// 	Compute the product of all components 
		inline uf::Vector<T,3>& negate();												// 	Flip sign of all components
	// 	Complex arithmetic
		inline T dot( const Vector<T,3> right ) const; 									// 	Compute the dot product between two vectors
		inline pod::Angle angle( const Vector<T,3>& b ) const; 							// 	Compute the angle between two vectors
		
		inline uf::Vector<T,3> lerp( const Vector<T,3> to, double delta ) const; 		// 	Linearly interpolate between two vectors
		inline uf::Vector<T,3> slerp( const Vector<T,3> to, double delta ) const; 		// 	Spherically interpolate between two vectors
		
		inline T distanceSquared( const Vector<T,3> b ) const; 							// 	Compute the distance between two vectors (doesn't sqrt)
		inline T distance( const Vector<T,3> b ) const; 								// 	Compute the distance between two vectors
		inline T magnitude() const; 													// 	Gets the magnitude of the vector
		inline T norm() const; 															// 	Compute the norm of the vector
		
		inline uf::Vector<T,3>& normalize(); 											// 	Normalizes a vector
		uf::Vector<T,3> getNormalized() const; 											// 	Return a normalized vector
		inline uf::stl::string toString() const;
	// 	Overloaded ops
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,3> operator-() const; 								// 	Negation
		inline Vector<T,3> operator+( const Vector<T,3>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,3> operator-( const Vector<T,3>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,3> operator*( const Vector<T,3>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,3> operator/( const Vector<T,3>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,3> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,3> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,3>& operator +=( const Vector<T,3>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,3>& operator -=( const Vector<T,3>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,3>& operator *=( const Vector<T,3>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,3>& operator /=( const Vector<T,3>& vector ); 		// 	Division set between two vectors
		inline Vector<T,3>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,3>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,3>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		
		inline operator pod_t&() { return this->m_pod; }
		inline operator const pod_t&() const { return this->m_pod; }
	};
	template<typename T>
	struct /*UF_API*/ Vector<T,4> {
	public:
	// 	Easily access POD's type
		typedef pod::Vector<T,4> pod_t;
	// 	Replicate POD information
		typedef T type_t;
		typedef T* container_t;
		static const std::size_t size = 4;
	protected:
	// 	POD storage
		Vector<T,4>::pod_t m_pod;
	public:
		T& x = m_pod.x;
		T& y = m_pod.y;
		T& z = m_pod.z;
		T& w = m_pod.w;

		T& r = m_pod.x;
		T& g = m_pod.y;
		T& b = m_pod.z;
		T& a = m_pod.w;
	public:
	// 	C-tor
		Vector(); 																		// initializes POD to 'def'
		Vector(T def); 																	// initializes POD to 'def'
		Vector(T x, T y, T z, T w); 													// initializes POD to 'def'
		Vector(const Vector<T,4>::pod_t& pod); 											// copies POD altogether
		Vector(const T components[4]); 													// copies data into POD from 'components' (typed as C array)
		Vector(const uf::stl::vector<T>& components); 										// copies data into POD from 'components' (typed as uf::stl::vector<T>)
	// 	D-tor
		// Unneccesary
	// 	POD access
		Vector<T,4>::pod_t& data(); 													// 	Returns a reference of POD
		const Vector<T,4>::pod_t& data() const; 										// 	Returns a const-reference of POD
	// 	Alternative POD access
		T* get();																		// 	Returns a pointer to the entire array
		const T* get() const; 															// 	Returns a const-pointer to the entire array
		T& getComponent( std::size_t i );												// 	Returns a reference to a single element
		const T& getComponent( std::size_t i ) const; 									// 	Returns a const-reference to a single element
	// 	POD manipulation
		T* set(const T components[4]);													// 	Sets the entire array
		T& setComponent( std::size_t i, const T& value );								// 	Sets a single element
	// 	Validation
		bool isValid() const; 															// 	Checks if all components are valid (non NaN, inf, etc.)
	// 	Basic arithmetic
		inline uf::Vector<T,4>& add( const Vector<T, 4>& vector );						// 	Adds two vectors of same type and size together
		inline uf::Vector<T,4>& subtract( const Vector<T, 4>& vector );					// 	Subtracts two vectors of same type and size together
		inline uf::Vector<T,4>& multiply( const Vector<T, 4>& vector );					// 	Multiplies two vectors of same type and size together
		inline uf::Vector<T,4>& multiply( T scalar );									// 	Multiplies this vector by a scalar
		inline uf::Vector<T,4>& divide( const Vector<T, 4>& vector );					// 	Divides two vectors of same type and size together
		inline uf::Vector<T,4>& divide( T scalar );										// 	Divides this vector by a scalar
		inline T sum() const;															// 	Compute the sum of all components 
		inline T product() const;														// 	Compute the product of all components 
		inline uf::Vector<T,4>& negate();												// 	Flip sign of all components
	// 	Complex arithmetic
		inline T dot( const Vector<T,4> right ) const; 									// 	Compute the dot product between two vectors
		inline pod::Angle angle( const Vector<T,4>& b ) const; 							// 	Compute the angle between two vectors
		
		inline uf::Vector<T,4> lerp( const Vector<T,4> to, double delta ) const; 		// 	Linearly interpolate between two vectors
		inline uf::Vector<T,4> slerp( const Vector<T,4> to, double delta ) const; 		// 	Spherically interpolate between two vectors
		
		inline T distanceSquared( const Vector<T,4> b ) const; 							// 	Compute the distance between two vectors (doesn't sqrt)
		inline T distance( const Vector<T,4> b ) const; 								// 	Compute the distance between two vectors
		inline T magnitude() const; 													// 	Gets the magnitude of the vector
		inline T norm() const; 															// 	Compute the norm of the vector
		
		inline uf::Vector<T,4>& normalize(); 											// 	Normalizes a vector
		uf::Vector<T,4> getNormalized() const; 											// 	Return a normalized vector
		inline uf::stl::string toString() const;
	// 	Overloaded ops
		// Accessing via subscripts
		T& operator[](std::size_t i);
		const T& operator[](std::size_t i) const;
		// Arithmetic
		inline Vector<T,4> operator-() const; 								// 	Negation
		inline Vector<T,4> operator+( const Vector<T,4>& vector ) const; 	// 	Addition between two vectors
		inline Vector<T,4> operator-( const Vector<T,4>& vector ) const; 	// 	Subtraction between two vectors
		inline Vector<T,4> operator*( const Vector<T,4>& vector ) const; 	// 	Multiplication between two vectors
		inline Vector<T,4> operator/( const Vector<T,4>& vector ) const; 	// 	Division between two vectors
		inline Vector<T,4> operator*( T scalar ) const; 					// 	Multiplication with scalar
		inline Vector<T,4> operator/( T scalar ) const; 					// 	Division with scalar
		inline Vector<T,4>& operator +=( const Vector<T,4>& vector ); 		// 	Addition set between two vectors
		inline Vector<T,4>& operator -=( const Vector<T,4>& vector ); 		// 	Subtraction set between two vectors
		inline Vector<T,4>& operator *=( const Vector<T,4>& vector ); 		// 	Multiplication set between two vectors
		inline Vector<T,4>& operator /=( const Vector<T,4>& vector ); 		// 	Division set between two vectors
		inline Vector<T,4>& operator *=( T scalar ); 						// 	Multiplication set with scalar
		inline Vector<T,4>& operator /=( T scalar ); 						// 	Division set with scalar
		inline bool operator==( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (equals)
		inline bool operator!=( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (not equals)
		inline bool operator<( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (less than)
		inline bool operator<=( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (less than or equals)
		inline bool operator>( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (greater than)
		inline bool operator>=( const Vector<T,4>& vector ) const; 			// 	Equality check between two vectors (greater than or equals)
		
		inline operator pod_t&() { return this->m_pod; }
		inline operator const pod_t&() const { return this->m_pod; }
	};
}
// 	C-tor
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,1>::Vector() {
}
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,1>::Vector(T def) {
	for ( std::size_t i = 0; i < 1; ++i ) this->m_pod[i] = def;
}
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,1>::Vector(T x, T y) {
	this->m_pod.x = x;
	this->m_pod.y = y;
}
template<typename T> 														// copies POD altogether
uf::Vector<T,1>::Vector(const typename uf::Vector<T,1>::pod_t& pod) : m_pod(pod) {
}
template<typename T> 														// copies data into POD from 'components' (typed as C array)
uf::Vector<T,1>::Vector(const T components[1]) {
	this->set(&components[0]);
}
template<typename T> 														// copies data into POD from 'components' (typed as uf::stl::vector<T>)
uf::Vector<T,1>::Vector(const uf::stl::vector<T>& components) {
	if ( components.size() >= 1 ) this->set(&components[0]);
}
// 	D-tor
// Unneccesary
// 	POD access
template<typename T> 														// 	Returns a reference of POD
typename uf::Vector<T,1>::pod_t& uf::Vector<T,1>::data() {
	return this->m_pod;
}
template<typename T> 														// 	Returns a const-reference of POD
const typename uf::Vector<T,1>::pod_t& uf::Vector<T,1>::data() const {
	return this->m_pod;
}
// 	Alternative POD access
template<typename T> 														// 	Returns a pointer to the entire array
T* uf::Vector<T,1>::get() {
	return &this->m_pod[0];
}
template<typename T> 														// 	Returns a const-pointer to the entire array
const T* uf::Vector<T,1>::get() const {
	return &this->m_pod[0];
}
template<typename T> 														// 	Returns a reference to a single element
T& uf::Vector<T,1>::getComponent( std::size_t i ) {
	if ( i >= 1 ) return NULL;
	return this->m_pod[i];
}
template<typename T> 														// 	Returns a const-reference to a single element
const T& uf::Vector<T,1>::getComponent( std::size_t i ) const {
	if ( i >= 1 ) return NULL;
	return this->m_pod[i];
}
// 	POD manipulation
template<typename T> 														// 	Sets the entire array
T* uf::Vector<T,1>::set(const T components[1]) {
	for ( std::size_t i = 0; i < 1; ++i ) this->m_pod[i] = components[i];
	return &this->m_pod[0];
}
template<typename T> 														// 	Sets a single element
T& uf::Vector<T,1>::setComponent( std::size_t i, const T& value ) {
	this->m_pod[i] = value;
}
// 	Validation
template<typename T> 														// 	Checks if all components are valid (non NaN, inf, etc.)
bool uf::Vector<T,1>::isValid() const {
	for ( std::size_t i = 0; i < 1; ++i ) if ( this->m_pod[i] != this->m_pod[i] ) return false;
	return true;
}
// 	Basic arithmetic
template<typename T> 														// 	Adds two vectors of same type and size together
inline uf::Vector<T,1>& uf::Vector<T,1>::add( const uf::Vector<T, 1>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T> 														// 	Subtracts two vectors of same type and size together
inline uf::Vector<T,1>& uf::Vector<T,1>::subtract( const uf::Vector<T, 1>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T> 														// 	Multiplies two vectors of same type and size together
inline uf::Vector<T,1>& uf::Vector<T,1>::multiply( const uf::Vector<T, 1>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T> 														// 	Multiplies this vector by a scalar
inline uf::Vector<T,1>& uf::Vector<T,1>::multiply( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T> 														// 	Divides two vectors of same type and size together
inline uf::Vector<T,1>& uf::Vector<T,1>::divide( const uf::Vector<T, 1>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T> 														// 	Divides this vector by a scalar
inline uf::Vector<T,1>& uf::Vector<T,1>::divide( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T> 														// 	Compute the sum of all components 
inline T uf::Vector<T,1>::sum() const {
	return uf::vector::sum( this->m_pod );
}
template<typename T> 														// 	Compute the product of all components 
inline T uf::Vector<T,1>::product() const {
	return uf::vector::product( this->m_pod );
}
template<typename T> 														// 	Flip sign of all components
inline uf::Vector<T,1>& uf::Vector<T,1>::negate() {
	return uf::vector::negate( this->m_pod );
}
// 	Complex arithmetic
template<typename T> 														// 	Compute the dot product between two vectors
inline T uf::Vector<T,1>::dot( const uf::Vector<T,1> right ) const {
	return uf::vector::dot( this->m_pod, right );
}
template<typename T> 														// 	Compute the angle between two vectors
inline pod::Angle uf::Vector<T,1>::angle( const uf::Vector<T,1>& b ) const {
	return uf::vector::angle( this->m_pod, b );
}

template<typename T> 														// 	Linearly interpolate between two vectors
inline uf::Vector<T,1> uf::Vector<T,1>::lerp( const uf::Vector<T,1> to, double delta ) const {
	return uf::vector::lerp( this->m_pod, to, delta );
}
template<typename T> 														// 	Spherically interpolate between two vectors
inline uf::Vector<T,1> uf::Vector<T,1>::slerp( const uf::Vector<T,1> to, double delta ) const {
	return uf::vector::slerp( this->m_pod, to, delta );
}

template<typename T> 														// 	Compute the distance between two vectors (doesn't sqrt)
inline T uf::Vector<T,1>::distanceSquared( const uf::Vector<T,1> b ) const {
	return uf::vector::distanceSquared( this->m_pod, b );
}
template<typename T> 														// 	Compute the distance between two vectors
inline T uf::Vector<T,1>::distance( const uf::Vector<T,1> b ) const {
	return uf::vector::distance( this->m_pod, b );
}
template<typename T> 														// 	Gets the magnitude of the vector
inline T uf::Vector<T,1>::magnitude() const {
	return uf::vector::magnitude( this->m_pod );
}
template<typename T> 														// 	Compute the norm of the vector
inline T uf::Vector<T,1>::norm() const {
	return uf::vector::norm( this->m_pod );
}

template<typename T> 														// 	Normalizes a vector
inline uf::Vector<T,1>& uf::Vector<T,1>::normalize() {
	return uf::vector::normalize( this->m_pod );
}
template<typename T> 														// 	Return a normalized vector
uf::Vector<T,1> uf::Vector<T,1>::getNormalized() const {
	return uf::vector::normalize( this->m_pod );
}
template<typename T> 														// 	Return a string
uf::stl::string uf::Vector<T,1>::toString() const {
	return uf::vector::toString( this->m_pod );
}
// Overloaded ops
template<typename T> 
T& uf::Vector<T,1>::operator[](std::size_t i) {
	if ( i >= 1 ) return NULL;
	return this->m_pod[i];
}
template<typename T> 
const T& uf::Vector<T,1>::operator[](std::size_t i) const {
	if ( i >= 1 ) return NULL;
	return this->m_pod[i];
}
// Arithmetic
template<typename T>  															// 	Negation
inline uf::Vector<T,1> uf::Vector<T,1>::operator-() const {
	return uf::vector::negate( this->m_pod );
}			
template<typename T>  															// 	Addition between two vectors
inline uf::Vector<T,1> uf::Vector<T,1>::operator+( const uf::Vector<T,1>& vector ) const {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T>  															// 	Subtraction between two vectors
inline uf::Vector<T,1> uf::Vector<T,1>::operator-( const uf::Vector<T,1>& vector ) const {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication between two vectors
inline uf::Vector<T,1> uf::Vector<T,1>::operator*( const uf::Vector<T,1>& vector ) const {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T>  															// 	Division between two vectors
inline uf::Vector<T,1> uf::Vector<T,1>::operator/( const uf::Vector<T,1>& vector ) const {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication with scalar
inline uf::Vector<T,1> uf::Vector<T,1>::operator*( T scalar ) const {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T>  															// 	Division with scalar
inline uf::Vector<T,1> uf::Vector<T,1>::operator/( T scalar ) const {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T>  															// 	Addition set between two vectors
inline uf::Vector<T,1>& uf::Vector<T,1>::operator +=( const uf::Vector<T,1>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T>  															// 	Subtraction set between two vectors
inline uf::Vector<T,1>& uf::Vector<T,1>::operator -=( const uf::Vector<T,1>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication set between two vectors
inline uf::Vector<T,1>& uf::Vector<T,1>::operator *=( const uf::Vector<T,1>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T>  															// 	Division set between two vectors
inline uf::Vector<T,1>& uf::Vector<T,1>::operator /=( const uf::Vector<T,1>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication set with scalar
inline uf::Vector<T,1>& uf::Vector<T,1>::operator *=( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T>  															// 	Division set with scalar
inline uf::Vector<T,1>& uf::Vector<T,1>::operator /=( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T>  															// 	Equality check between two vectors (equals)
inline bool uf::Vector<T,1>::operator==( const uf::Vector<T,1>& vector ) const {
	return uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T>  															// 	Equality check between two vectors (not equals)
inline bool uf::Vector<T,1>::operator!=( const uf::Vector<T,1>& vector ) const {
	return !uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T>  															// 	Equality check between two vectors (less than)
inline bool uf::Vector<T,1>::operator<( const uf::Vector<T,1>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) < 0;
}
template<typename T>  															// 	Equality check between two vectors (less than or equals)
inline bool uf::Vector<T,1>::operator<=( const uf::Vector<T,1>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) <= 0;
}
template<typename T>  															// 	Equality check between two vectors (greater than)
inline bool uf::Vector<T,1>::operator>( const uf::Vector<T,1>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) > 0;
}
template<typename T>  															// 	Equality check between two vectors (greater than or equals)
inline bool uf::Vector<T,1>::operator>=( const uf::Vector<T,1>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) >= 0;
}
// 	C-tor
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,2>::Vector() {
}
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,2>::Vector(T def) {
	for ( std::size_t i = 0; i < 2; ++i ) this->m_pod[i] = def;
}
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,2>::Vector(T x, T y) {
	this->m_pod.x = x;
	this->m_pod.y = y;
}
template<typename T> 														// copies POD altogether
uf::Vector<T,2>::Vector(const typename uf::Vector<T,2>::pod_t& pod) : m_pod(pod) {
}
template<typename T> 														// copies data into POD from 'components' (typed as C array)
uf::Vector<T,2>::Vector(const T components[2]) {
	this->set(&components[0]);
}
template<typename T> 														// copies data into POD from 'components' (typed as uf::stl::vector<T>)
uf::Vector<T,2>::Vector(const uf::stl::vector<T>& components) {
	if ( components.size() >= 2 ) this->set(&components[0]);
}
// 	D-tor
// Unneccesary
// 	POD access
template<typename T> 														// 	Returns a reference of POD
typename uf::Vector<T,2>::pod_t& uf::Vector<T,2>::data() {
	return this->m_pod;
}
template<typename T> 														// 	Returns a const-reference of POD
const typename uf::Vector<T,2>::pod_t& uf::Vector<T,2>::data() const {
	return this->m_pod;
}
// 	Alternative POD access
template<typename T> 														// 	Returns a pointer to the entire array
T* uf::Vector<T,2>::get() {
	return &this->m_pod[0];
}
template<typename T> 														// 	Returns a const-pointer to the entire array
const T* uf::Vector<T,2>::get() const {
	return &this->m_pod[0];
}
template<typename T> 														// 	Returns a reference to a single element
T& uf::Vector<T,2>::getComponent( std::size_t i ) {
	if ( i >= 2 ) return NULL;
	return this->m_pod[i];
}
template<typename T> 														// 	Returns a const-reference to a single element
const T& uf::Vector<T,2>::getComponent( std::size_t i ) const {
	if ( i >= 2 ) return NULL;
	return this->m_pod[i];
}
// 	POD manipulation
template<typename T> 														// 	Sets the entire array
T* uf::Vector<T,2>::set(const T components[2]) {
	for ( std::size_t i = 0; i < 2; ++i ) this->m_pod[i] = components[i];
	return &this->m_pod[0];
}
template<typename T> 														// 	Sets a single element
T& uf::Vector<T,2>::setComponent( std::size_t i, const T& value ) {
	this->m_pod[i] = value;
}
// 	Validation
template<typename T> 														// 	Checks if all components are valid (non NaN, inf, etc.)
bool uf::Vector<T,2>::isValid() const {
	for ( std::size_t i = 0; i < 2; ++i ) if ( this->m_pod[i] != this->m_pod[i] ) return false;
	return true;
}
// 	Basic arithmetic
template<typename T> 														// 	Adds two vectors of same type and size together
inline uf::Vector<T,2>& uf::Vector<T,2>::add( const uf::Vector<T, 2>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T> 														// 	Subtracts two vectors of same type and size together
inline uf::Vector<T,2>& uf::Vector<T,2>::subtract( const uf::Vector<T, 2>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T> 														// 	Multiplies two vectors of same type and size together
inline uf::Vector<T,2>& uf::Vector<T,2>::multiply( const uf::Vector<T, 2>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T> 														// 	Multiplies this vector by a scalar
inline uf::Vector<T,2>& uf::Vector<T,2>::multiply( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T> 														// 	Divides two vectors of same type and size together
inline uf::Vector<T,2>& uf::Vector<T,2>::divide( const uf::Vector<T, 2>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T> 														// 	Divides this vector by a scalar
inline uf::Vector<T,2>& uf::Vector<T,2>::divide( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T> 														// 	Compute the sum of all components 
inline T uf::Vector<T,2>::sum() const {
	return uf::vector::sum( this->m_pod );
}
template<typename T> 														// 	Compute the product of all components 
inline T uf::Vector<T,2>::product() const {
	return uf::vector::product( this->m_pod );
}
template<typename T> 														// 	Flip sign of all components
inline uf::Vector<T,2>& uf::Vector<T,2>::negate() {
	return uf::vector::negate( this->m_pod );
}
// 	Complex arithmetic
template<typename T> 														// 	Compute the dot product between two vectors
inline T uf::Vector<T,2>::dot( const uf::Vector<T,2> right ) const {
	return uf::vector::dot( this->m_pod, right );
}
template<typename T> 														// 	Compute the angle between two vectors
inline pod::Angle uf::Vector<T,2>::angle( const uf::Vector<T,2>& b ) const {
	return uf::vector::angle( this->m_pod, b );
}

template<typename T> 														// 	Linearly interpolate between two vectors
inline uf::Vector<T,2> uf::Vector<T,2>::lerp( const uf::Vector<T,2> to, double delta ) const {
	return uf::vector::lerp( this->m_pod, to, delta );
}
template<typename T> 														// 	Spherically interpolate between two vectors
inline uf::Vector<T,2> uf::Vector<T,2>::slerp( const uf::Vector<T,2> to, double delta ) const {
	return uf::vector::slerp( this->m_pod, to, delta );
}

template<typename T> 														// 	Compute the distance between two vectors (doesn't sqrt)
inline T uf::Vector<T,2>::distanceSquared( const uf::Vector<T,2> b ) const {
	return uf::vector::distanceSquared( this->m_pod, b );
}
template<typename T> 														// 	Compute the distance between two vectors
inline T uf::Vector<T,2>::distance( const uf::Vector<T,2> b ) const {
	return uf::vector::distance( this->m_pod, b );
}
template<typename T> 														// 	Gets the magnitude of the vector
inline T uf::Vector<T,2>::magnitude() const {
	return uf::vector::magnitude( this->m_pod );
}
template<typename T> 														// 	Compute the norm of the vector
inline T uf::Vector<T,2>::norm() const {
	return uf::vector::norm( this->m_pod );
}

template<typename T> 														// 	Normalizes a vector
inline uf::Vector<T,2>& uf::Vector<T,2>::normalize() {
	return uf::vector::normalize( this->m_pod );
}
template<typename T> 														// 	Return a normalized vector
uf::Vector<T,2> uf::Vector<T,2>::getNormalized() const {
	return uf::vector::normalize( this->m_pod );
}
template<typename T> 														// 	Return a string
uf::stl::string uf::Vector<T,2>::toString() const {
	return uf::vector::toString( this->m_pod );
}
// Overloaded ops
template<typename T> 
T& uf::Vector<T,2>::operator[](std::size_t i) {
	if ( i >= 2 ) return NULL;
	return this->m_pod[i];
}
template<typename T> 
const T& uf::Vector<T,2>::operator[](std::size_t i) const {
	if ( i >= 2 ) return NULL;
	return this->m_pod[i];
}
// Arithmetic
template<typename T>  															// 	Negation
inline uf::Vector<T,2> uf::Vector<T,2>::operator-() const {
	return uf::vector::negate( this->m_pod );
}			
template<typename T>  															// 	Addition between two vectors
inline uf::Vector<T,2> uf::Vector<T,2>::operator+( const uf::Vector<T,2>& vector ) const {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T>  															// 	Subtraction between two vectors
inline uf::Vector<T,2> uf::Vector<T,2>::operator-( const uf::Vector<T,2>& vector ) const {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication between two vectors
inline uf::Vector<T,2> uf::Vector<T,2>::operator*( const uf::Vector<T,2>& vector ) const {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T>  															// 	Division between two vectors
inline uf::Vector<T,2> uf::Vector<T,2>::operator/( const uf::Vector<T,2>& vector ) const {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication with scalar
inline uf::Vector<T,2> uf::Vector<T,2>::operator*( T scalar ) const {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T>  															// 	Division with scalar
inline uf::Vector<T,2> uf::Vector<T,2>::operator/( T scalar ) const {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T>  															// 	Addition set between two vectors
inline uf::Vector<T,2>& uf::Vector<T,2>::operator +=( const uf::Vector<T,2>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T>  															// 	Subtraction set between two vectors
inline uf::Vector<T,2>& uf::Vector<T,2>::operator -=( const uf::Vector<T,2>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication set between two vectors
inline uf::Vector<T,2>& uf::Vector<T,2>::operator *=( const uf::Vector<T,2>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T>  															// 	Division set between two vectors
inline uf::Vector<T,2>& uf::Vector<T,2>::operator /=( const uf::Vector<T,2>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication set with scalar
inline uf::Vector<T,2>& uf::Vector<T,2>::operator *=( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T>  															// 	Division set with scalar
inline uf::Vector<T,2>& uf::Vector<T,2>::operator /=( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T>  															// 	Equality check between two vectors (equals)
inline bool uf::Vector<T,2>::operator==( const uf::Vector<T,2>& vector ) const {
	return uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T>  															// 	Equality check between two vectors (not equals)
inline bool uf::Vector<T,2>::operator!=( const uf::Vector<T,2>& vector ) const {
	return !uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T>  															// 	Equality check between two vectors (less than)
inline bool uf::Vector<T,2>::operator<( const uf::Vector<T,2>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) < 0;
}
template<typename T>  															// 	Equality check between two vectors (less than or equals)
inline bool uf::Vector<T,2>::operator<=( const uf::Vector<T,2>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) <= 0;
}
template<typename T>  															// 	Equality check between two vectors (greater than)
inline bool uf::Vector<T,2>::operator>( const uf::Vector<T,2>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) > 0;
}
template<typename T>  															// 	Equality check between two vectors (greater than or equals)
inline bool uf::Vector<T,2>::operator>=( const uf::Vector<T,2>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) >= 0;
}
//
// 	C-tor
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,3>::Vector() {
}
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,3>::Vector(T def) {
	for ( std::size_t i = 0; i < 3; ++i ) this->m_pod[i] = def;
}
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,3>::Vector(T x, T y, T z) {
	this->m_pod.x = x;
	this->m_pod.y = y;
	this->m_pod.z = z;
}
template<typename T> 														// copies POD altogether
uf::Vector<T,3>::Vector(const typename uf::Vector<T,3>::pod_t& pod) : m_pod(pod) {
}
template<typename T> 														// copies data into POD from 'components' (typed as C array)
uf::Vector<T,3>::Vector(const T components[3]) {
	this->set(&components[0]);
}
template<typename T> 														// copies data into POD from 'components' (typed as uf::stl::vector<T>)
uf::Vector<T,3>::Vector(const uf::stl::vector<T>& components) {
	if ( components.size() >= 3 ) this->set(&components[0]);
}
// 	D-tor
// Unneccesary
// 	POD access
template<typename T> 														// 	Returns a reference of POD
typename uf::Vector<T,3>::pod_t& uf::Vector<T,3>::data() {
	return this->m_pod;
}
template<typename T> 														// 	Returns a const-reference of POD
const typename uf::Vector<T,3>::pod_t& uf::Vector<T,3>::data() const {
	return this->m_pod;
}
// 	Alternative POD access
template<typename T> 														// 	Returns a pointer to the entire array
T* uf::Vector<T,3>::get() {
	return &this->m_pod[0];
}
template<typename T> 														// 	Returns a const-pointer to the entire array
const T* uf::Vector<T,3>::get() const {
	return &this->m_pod[0];
}
template<typename T> 														// 	Returns a reference to a single element
T& uf::Vector<T,3>::getComponent( std::size_t i ) {
	if ( i >= 3 ) return NULL;
	return this->m_pod[i];
}
template<typename T> 														// 	Returns a const-reference to a single element
const T& uf::Vector<T,3>::getComponent( std::size_t i ) const {
	if ( i >= 3 ) return NULL;
	return this->m_pod[i];
}
// 	POD manipulation
template<typename T> 														// 	Sets the entire array
T* uf::Vector<T,3>::set(const T components[3]) {
	for ( std::size_t i = 0; i < 3; ++i ) this->m_pod[i] = components[i];
	return &this->m_pod[0];
}
template<typename T> 														// 	Sets a single element
T& uf::Vector<T,3>::setComponent( std::size_t i, const T& value ) {
	this->m_pod[i] = value;
}
// 	Validation
template<typename T> 														// 	Checks if all components are valid (non NaN, inf, etc.)
bool uf::Vector<T,3>::isValid() const {
	for ( std::size_t i = 0; i < 3; ++i ) if ( this->m_pod[i] != this->m_pod[i] ) return false;
	return true;
}
// 	Basic arithmetic
template<typename T> 														// 	Adds two vectors of same type and size together
inline uf::Vector<T,3>& uf::Vector<T,3>::add( const uf::Vector<T,3>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T> 														// 	Subtracts two vectors of same type and size together
inline uf::Vector<T,3>& uf::Vector<T,3>::subtract( const uf::Vector<T,3>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T> 														// 	Multiplies two vectors of same type and size together
inline uf::Vector<T,3>& uf::Vector<T,3>::multiply( const uf::Vector<T,3>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T> 														// 	Multiplies this vector by a scalar
inline uf::Vector<T,3>& uf::Vector<T,3>::multiply( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T> 														// 	Divides two vectors of same type and size together
inline uf::Vector<T,3>& uf::Vector<T,3>::divide( const uf::Vector<T,3>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T> 														// 	Divides this vector by a scalar
inline uf::Vector<T,3>& uf::Vector<T,3>::divide( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T> 														// 	Compute the sum of all components 
inline T uf::Vector<T,3>::sum() const {
	return uf::vector::sum( this->m_pod );
}
template<typename T> 														// 	Compute the product of all components 
inline T uf::Vector<T,3>::product() const {
	return uf::vector::product( this->m_pod );
}
template<typename T> 														// 	Flip sign of all components
inline uf::Vector<T,3>& uf::Vector<T,3>::negate() {
	return uf::vector::negate( this->m_pod );
}
// 	Complex arithmetic
template<typename T> 														// 	Compute the dot product between two vectors
inline T uf::Vector<T,3>::dot( const uf::Vector<T,3> right ) const {
	return uf::vector::dot( this->m_pod, right );
}
template<typename T> 														// 	Compute the angle between two vectors
inline pod::Angle uf::Vector<T,3>::angle( const uf::Vector<T,3>& b ) const {
	return uf::vector::angle( this->m_pod, b );
}

template<typename T> 														// 	Linearly interpolate between two vectors
inline uf::Vector<T,3> uf::Vector<T,3>::lerp( const uf::Vector<T,3> to, double delta ) const {
	return uf::vector::lerp( this->m_pod, to, delta );
}
template<typename T> 														// 	Spherically interpolate between two vectors
inline uf::Vector<T,3> uf::Vector<T,3>::slerp( const uf::Vector<T,3> to, double delta ) const {
	return uf::vector::slerp( this->m_pod, to, delta );
}

template<typename T> 														// 	Compute the distance between two vectors (doesn't sqrt)
inline T uf::Vector<T,3>::distanceSquared( const uf::Vector<T,3> b ) const {
	return uf::vector::distanceSquared( this->m_pod, b );
}
template<typename T> 														// 	Compute the distance between two vectors
inline T uf::Vector<T,3>::distance( const uf::Vector<T,3> b ) const {
	return uf::vector::distance( this->m_pod, b );
}
template<typename T> 														// 	Gets the magnitude of the vector
inline T uf::Vector<T,3>::magnitude() const {
	return uf::vector::magnitude( this->m_pod );
}
template<typename T> 														// 	Compute the norm of the vector
inline T uf::Vector<T,3>::norm() const {
	return uf::vector::norm( this->m_pod );
}

template<typename T> 														// 	Normalizes a vector
inline uf::Vector<T,3>& uf::Vector<T,3>::normalize() {
	return uf::vector::normalize( this->m_pod );
}
template<typename T> 														// 	Return a normalized vector
uf::Vector<T,3> uf::Vector<T,3>::getNormalized() const {
	return uf::vector::normalize( this->m_pod );
}
template<typename T> 														// 	Return a string
uf::stl::string uf::Vector<T,3>::toString() const {
	return uf::vector::toString( this->m_pod );
}
// Overloaded ops
template<typename T> 
T& uf::Vector<T,3>::operator[](std::size_t i) {
	if ( i >= 3 ) return NULL;
	return this->m_pod[i];
}
template<typename T> 
const T& uf::Vector<T,3>::operator[](std::size_t i) const {
	if ( i >= 3 ) return NULL;
	return this->m_pod[i];
}
// Arithmetic
template<typename T>  															// 	Negation
inline uf::Vector<T,3> uf::Vector<T,3>::operator-() const {
	return uf::vector::negate( this->m_pod );
}			
template<typename T>  															// 	Addition between two vectors
inline uf::Vector<T,3> uf::Vector<T,3>::operator+( const uf::Vector<T,3>& vector ) const {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T>  															// 	Subtraction between two vectors
inline uf::Vector<T,3> uf::Vector<T,3>::operator-( const uf::Vector<T,3>& vector ) const {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication between two vectors
inline uf::Vector<T,3> uf::Vector<T,3>::operator*( const uf::Vector<T,3>& vector ) const {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T>  															// 	Division between two vectors
inline uf::Vector<T,3> uf::Vector<T,3>::operator/( const uf::Vector<T,3>& vector ) const {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication with scalar
inline uf::Vector<T,3> uf::Vector<T,3>::operator*( T scalar ) const {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T>  															// 	Division with scalar
inline uf::Vector<T,3> uf::Vector<T,3>::operator/( T scalar ) const {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T>  															// 	Addition set between two vectors
inline uf::Vector<T,3>& uf::Vector<T,3>::operator +=( const uf::Vector<T,3>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T>  															// 	Subtraction set between two vectors
inline uf::Vector<T,3>& uf::Vector<T,3>::operator -=( const uf::Vector<T,3>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication set between two vectors
inline uf::Vector<T,3>& uf::Vector<T,3>::operator *=( const uf::Vector<T,3>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T>  															// 	Division set between two vectors
inline uf::Vector<T,3>& uf::Vector<T,3>::operator /=( const uf::Vector<T,3>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication set with scalar
inline uf::Vector<T,3>& uf::Vector<T,3>::operator *=( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T>  															// 	Division set with scalar
inline uf::Vector<T,3>& uf::Vector<T,3>::operator /=( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T>  															// 	Equality check between two vectors (equals)
inline bool uf::Vector<T,3>::operator==( const uf::Vector<T,3>& vector ) const {
	return uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T>  															// 	Equality check between two vectors (not equals)
inline bool uf::Vector<T,3>::operator!=( const uf::Vector<T,3>& vector ) const {
	return !uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T>  															// 	Equality check between two vectors (less than)
inline bool uf::Vector<T,3>::operator<( const uf::Vector<T,3>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) < 0;
}
template<typename T>  															// 	Equality check between two vectors (less than or equals)
inline bool uf::Vector<T,3>::operator<=( const uf::Vector<T,3>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) <= 0;
}
template<typename T>  															// 	Equality check between two vectors (greater than)
inline bool uf::Vector<T,3>::operator>( const uf::Vector<T,3>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) > 0;
}
template<typename T>  															// 	Equality check between two vectors (greater than or equals)
inline bool uf::Vector<T,3>::operator>=( const uf::Vector<T,3>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) >= 0;
}
//
// 	C-tor
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,4>::Vector() {
}
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,4>::Vector(T def) {
	for ( std::size_t i = 0; i < 4; ++i ) this->m_pod[i] = def;
}
template<typename T> 														// initializes POD to 'def'
uf::Vector<T,4>::Vector(T x, T y, T z, T w) {
	this->m_pod.x = x;
	this->m_pod.y = y;
	this->m_pod.z = z;
	this->m_pod.w = w;
}
template<typename T> 														// copies POD altogether
uf::Vector<T,4>::Vector(const typename uf::Vector<T,4>::pod_t& pod) : m_pod(pod) {
}
template<typename T> 														// copies data into POD from 'components' (typed as C array)
uf::Vector<T,4>::Vector(const T components[4]) {
	this->set(&components[0]);
}
template<typename T> 														// copies data into POD from 'components' (typed as uf::stl::vector<T>)
uf::Vector<T,4>::Vector(const uf::stl::vector<T>& components) {
	if ( components.size() >= 4 ) this->set(&components[0]);
}
// 	D-tor
// Unneccesary
// 	POD access
template<typename T> 														// 	Returns a reference of POD
typename uf::Vector<T,4>::pod_t& uf::Vector<T,4>::data() {
	return this->m_pod;
}
template<typename T> 														// 	Returns a const-reference of POD
const typename uf::Vector<T,4>::pod_t& uf::Vector<T,4>::data() const {
	return this->m_pod;
}
// 	Alternative POD access
template<typename T> 														// 	Returns a pointer to the entire array
T* uf::Vector<T,4>::get() {
	return &this->m_pod[0];
}
template<typename T> 														// 	Returns a const-pointer to the entire array
const T* uf::Vector<T,4>::get() const {
	return &this->m_pod[0];
}
template<typename T> 														// 	Returns a reference to a single element
T& uf::Vector<T,4>::getComponent( std::size_t i ) {
	if ( i >= 4 ) return NULL;
	return this->m_pod[i];
}
template<typename T> 														// 	Returns a const-reference to a single element
const T& uf::Vector<T,4>::getComponent( std::size_t i ) const {
	if ( i >= 4 ) return NULL;
	return this->m_pod[i];
}
// 	POD manipulation
template<typename T> 														// 	Sets the entire array
T* uf::Vector<T,4>::set(const T components[4]) {
	for ( std::size_t i = 0; i < 4; ++i ) this->m_pod[i] = components[i];
	return &this->m_pod[0];
}
template<typename T> 														// 	Sets a single element
T& uf::Vector<T,4>::setComponent( std::size_t i, const T& value ) {
	this->m_pod[i] = value;
}
// 	Validation
template<typename T> 														// 	Checks if all components are valid (non NaN, inf, etc.)
bool uf::Vector<T,4>::isValid() const {
	for ( std::size_t i = 0; i < 4; ++i ) if ( this->m_pod[i] != this->m_pod[i] ) return false;
	return true;
}
// 	Basic arithmetic
template<typename T> 														// 	Adds two vectors of same type and size together
inline uf::Vector<T,4>& uf::Vector<T,4>::add( const uf::Vector<T, 4>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T> 														// 	Subtracts two vectors of same type and size together
inline uf::Vector<T,4>& uf::Vector<T,4>::subtract( const uf::Vector<T, 4>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T> 														// 	Multiplies two vectors of same type and size together
inline uf::Vector<T,4>& uf::Vector<T,4>::multiply( const uf::Vector<T, 4>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T> 														// 	Multiplies this vector by a scalar
inline uf::Vector<T,4>& uf::Vector<T,4>::multiply( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T> 														// 	Divides two vectors of same type and size together
inline uf::Vector<T,4>& uf::Vector<T,4>::divide( const uf::Vector<T, 4>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T> 														// 	Divides this vector by a scalar
inline uf::Vector<T,4>& uf::Vector<T,4>::divide( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T> 														// 	Compute the sum of all components 
inline T uf::Vector<T,4>::sum() const {
	return uf::vector::sum( this->m_pod );
}
template<typename T> 														// 	Compute the product of all components 
inline T uf::Vector<T,4>::product() const {
	return uf::vector::product( this->m_pod );
}
template<typename T> 														// 	Flip sign of all components
inline uf::Vector<T,4>& uf::Vector<T,4>::negate() {
	return uf::vector::negate( this->m_pod );
}
// 	Complex arithmetic
template<typename T> 														// 	Compute the dot product between two vectors
inline T uf::Vector<T,4>::dot( const uf::Vector<T,4> right ) const {
	return uf::vector::dot( this->m_pod, right );
}
template<typename T> 														// 	Compute the angle between two vectors
inline pod::Angle uf::Vector<T,4>::angle( const uf::Vector<T,4>& b ) const {
	return uf::vector::angle( this->m_pod, b );
}

template<typename T> 														// 	Linearly interpolate between two vectors
inline uf::Vector<T,4> uf::Vector<T,4>::lerp( const uf::Vector<T,4> to, double delta ) const {
	return uf::vector::lerp( this->m_pod, to, delta );
}
template<typename T> 														// 	Spherically interpolate between two vectors
inline uf::Vector<T,4> uf::Vector<T,4>::slerp( const uf::Vector<T,4> to, double delta ) const {
	return uf::vector::slerp( this->m_pod, to, delta );
}

template<typename T> 														// 	Compute the distance between two vectors (doesn't sqrt)
inline T uf::Vector<T,4>::distanceSquared( const uf::Vector<T,4> b ) const {
	return uf::vector::distanceSquared( this->m_pod, b );
}
template<typename T> 														// 	Compute the distance between two vectors
inline T uf::Vector<T,4>::distance( const uf::Vector<T,4> b ) const {
	return uf::vector::distance( this->m_pod, b );
}
template<typename T> 														// 	Gets the magnitude of the vector
inline T uf::Vector<T,4>::magnitude() const {
	return uf::vector::magnitude( this->m_pod );
}
template<typename T> 														// 	Compute the norm of the vector
inline T uf::Vector<T,4>::norm() const {
	return uf::vector::norm( this->m_pod );
}

template<typename T> 														// 	Normalizes a vector
inline uf::Vector<T,4>& uf::Vector<T,4>::normalize() {
	return uf::vector::normalize( this->m_pod );
}
template<typename T> 														// 	Return a normalized vector
uf::Vector<T,4> uf::Vector<T,4>::getNormalized() const {
	return uf::vector::normalize( this->m_pod );
}
template<typename T> 														// 	Return a string
uf::stl::string uf::Vector<T,4>::toString() const {
	return uf::vector::toString( this->m_pod );
}
// Overloaded ops
template<typename T> 
T& uf::Vector<T,4>::operator[](std::size_t i) {
	if ( i >= 4 ) return NULL;
	return this->m_pod[i];
}
template<typename T> 
const T& uf::Vector<T,4>::operator[](std::size_t i) const {
	if ( i >= 4 ) return NULL;
	return this->m_pod[i];
}
// Arithmetic
template<typename T>  															// 	Negation
inline uf::Vector<T,4> uf::Vector<T,4>::operator-() const {
	return uf::vector::negate( this->m_pod );
}			
template<typename T>  															// 	Addition between two vectors
inline uf::Vector<T,4> uf::Vector<T,4>::operator+( const uf::Vector<T,4>& vector ) const {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T>  															// 	Subtraction between two vectors
inline uf::Vector<T,4> uf::Vector<T,4>::operator-( const uf::Vector<T,4>& vector ) const {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication between two vectors
inline uf::Vector<T,4> uf::Vector<T,4>::operator*( const uf::Vector<T,4>& vector ) const {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T>  															// 	Division between two vectors
inline uf::Vector<T,4> uf::Vector<T,4>::operator/( const uf::Vector<T,4>& vector ) const {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication with scalar
inline uf::Vector<T,4> uf::Vector<T,4>::operator*( T scalar ) const {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T>  															// 	Division with scalar
inline uf::Vector<T,4> uf::Vector<T,4>::operator/( T scalar ) const {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T>  															// 	Addition set between two vectors
inline uf::Vector<T,4>& uf::Vector<T,4>::operator +=( const uf::Vector<T,4>& vector ) {
	return uf::vector::add( this->m_pod, vector.data() );
}
template<typename T>  															// 	Subtraction set between two vectors
inline uf::Vector<T,4>& uf::Vector<T,4>::operator -=( const uf::Vector<T,4>& vector ) {
	return uf::vector::subtract( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication set between two vectors
inline uf::Vector<T,4>& uf::Vector<T,4>::operator *=( const uf::Vector<T,4>& vector ) {
	return uf::vector::multiply( this->m_pod, vector.data() );
}
template<typename T>  															// 	Division set between two vectors
inline uf::Vector<T,4>& uf::Vector<T,4>::operator /=( const uf::Vector<T,4>& vector ) {
	return uf::vector::divide( this->m_pod, vector.data() );
}
template<typename T>  															// 	Multiplication set with scalar
inline uf::Vector<T,4>& uf::Vector<T,4>::operator *=( T scalar ) {
	return uf::vector::multiply( this->m_pod, scalar );
}
template<typename T>  															// 	Division set with scalar
inline uf::Vector<T,4>& uf::Vector<T,4>::operator /=( T scalar ) {
	return uf::vector::divide( this->m_pod, scalar );
}
template<typename T>  															// 	Equality check between two vectors (equals)
inline bool uf::Vector<T,4>::operator==( const uf::Vector<T,4>& vector ) const {
	return uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T>  															// 	Equality check between two vectors (not equals)
inline bool uf::Vector<T,4>::operator!=( const uf::Vector<T,4>& vector ) const {
	return !uf::vector::equals(this->m_pod, vector.data() );
}
template<typename T>  															// 	Equality check between two vectors (less than)
inline bool uf::Vector<T,4>::operator<( const uf::Vector<T,4>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) < 0;
}
template<typename T>  															// 	Equality check between two vectors (less than or equals)
inline bool uf::Vector<T,4>::operator<=( const uf::Vector<T,4>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) <= 0;
}
template<typename T>  															// 	Equality check between two vectors (greater than)
inline bool uf::Vector<T,4>::operator>( const uf::Vector<T,4>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) > 0;
}
template<typename T>  															// 	Equality check between two vectors (greater than or equals)
inline bool uf::Vector<T,4>::operator>=( const uf::Vector<T,4>& vector ) const {
	return uf::vector::compareTo(this->m_pod, vector.data() ) >= 0;
}