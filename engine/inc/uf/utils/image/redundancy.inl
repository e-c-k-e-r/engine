//
template<typename T>
T& pod::Pixel<T,3>::operator[](std::size_t i) {
	return this->components[i];
}
template<typename T>
const T& pod::Pixel<T,3>::operator[](std::size_t i) const {
	return this->components[i];
}
// Arithmetic
template<typename T>																		// 	Negation
inline pod::Pixel<T,3> pod::Pixel<T,3>::operator-() const {
	return uf::pixel::negate( *this );
}
template<typename T>																		// 	Multiplication between two pixels
inline pod::Pixel<T,3> pod::Pixel<T,3>::operator*( const pod::Pixel<T,3>& pixel ) const {
	return uf::pixel::multiply( *this, pixel );
}
template<typename T>																		// 	Division between two pixels
inline pod::Pixel<T,3> pod::Pixel<T,3>::operator/( const pod::Pixel<T,3>& pixel ) const {
	return uf::pixel::divide( *this, pixel );
}
template<typename T>																		// 	Multiplication with scalar
inline pod::Pixel<T,3> pod::Pixel<T,3>::operator*( T scalar ) const {
	return uf::pixel::multiply( *this, scalar );
}
template<typename T>																		// 	Division with scalar
inline pod::Pixel<T,3> pod::Pixel<T,3>::operator/( T scalar ) const {
	return uf::pixel::divide( *this, scalar );
}
template<typename T>																		// 	Multiplication set between two pixels
inline pod::Pixel<T,3>& pod::Pixel<T,3>::operator *=( const pod::Pixel<T,3>& pixel ) {
	return uf::pixel::multiply( *this, pixel );
}
template<typename T>																		// 	Division set between two pixels
inline pod::Pixel<T,3>& pod::Pixel<T,3>::operator /=( const pod::Pixel<T,3>& pixel ) {
	return uf::pixel::divide( *this, pixel );
}
template<typename T>																		// 	Multiplication set with scalar
inline pod::Pixel<T,3>& pod::Pixel<T,3>::operator *=( T scalar ) {
	return uf::pixel::multiply( *this, scalar );
}
template<typename T>																		// 	Division set with scalar
inline pod::Pixel<T,3>& pod::Pixel<T,3>::operator /=( T scalar ) {
	return uf::pixel::divide( *this, scalar );
}
template<typename T>																		// 	Equality check between two pixels (equals)
inline bool pod::Pixel<T,3>::operator==( const pod::Pixel<T,3>& pixel ) const {
	return uf::pixel::equals(pixel);
}
template<typename T>																		// 	Equality check between two pixels (not equals)
inline bool pod::Pixel<T,3>::operator!=( const pod::Pixel<T,3>& pixel ) const {
	return !uf::pixel::equals(pixel);
}

//
template<typename T>
T& pod::Pixel<T,4>::operator[](std::size_t i) {
	return this->components[i];
}
template<typename T>
const T& pod::Pixel<T,4>::operator[](std::size_t i) const {
	return this->components[i];
}
// Arithmetic
template<typename T>																		// 	Negation
inline pod::Pixel<T,4> pod::Pixel<T,4>::operator-() const {
	return uf::pixel::negate( *this );
}
template<typename T>																		// 	Multiplication between two pixels
inline pod::Pixel<T,4> pod::Pixel<T,4>::operator*( const pod::Pixel<T,4>& pixel ) const {
	return uf::pixel::multiply( *this, pixel );
}
template<typename T>																		// 	Division between two pixels
inline pod::Pixel<T,4> pod::Pixel<T,4>::operator/( const pod::Pixel<T,4>& pixel ) const {
	return uf::pixel::divide( *this, pixel );
}
template<typename T>																		// 	Multiplication with scalar
inline pod::Pixel<T,4> pod::Pixel<T,4>::operator*( T scalar ) const {
	return uf::pixel::multiply( *this, scalar );
}
template<typename T>																		// 	Division with scalar
inline pod::Pixel<T,4> pod::Pixel<T,4>::operator/( T scalar ) const {
	return uf::pixel::divide( *this, scalar );
}
template<typename T>																		// 	Multiplication set between two pixels
inline pod::Pixel<T,4>& pod::Pixel<T,4>::operator *=( const pod::Pixel<T,4>& pixel ) {
	return uf::pixel::multiply( *this, pixel );
}
template<typename T>																		// 	Division set between two pixels
inline pod::Pixel<T,4>& pod::Pixel<T,4>::operator /=( const pod::Pixel<T,4>& pixel ) {
	return uf::pixel::divide( *this, pixel );
}
template<typename T>																		// 	Multiplication set with scalar
inline pod::Pixel<T,4>& pod::Pixel<T,4>::operator *=( T scalar ) {
	return uf::pixel::multiply( *this, scalar );
}
template<typename T>																		// 	Division set with scalar
inline pod::Pixel<T,4>& pod::Pixel<T,4>::operator /=( T scalar ) {
	return uf::pixel::divide( *this, scalar );
}
template<typename T>																		// 	Equality check between two pixels (equals)
inline bool pod::Pixel<T,4>::operator==( const pod::Pixel<T,4>& pixel ) const {
	return uf::pixel::equals(pixel);
}
template<typename T>																		// 	Equality check between two pixels (not equals)
inline bool pod::Pixel<T,4>::operator!=( const pod::Pixel<T,4>& pixel ) const {
	return !uf::pixel::equals(pixel);
}