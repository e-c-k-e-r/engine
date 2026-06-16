template<typename T>
uf::Time<T>::Time(uf::Time<T>::time_t t, uf::Time<T>::exp_t b ) : 
	m_time(t),
	m_exp(b)
{

}
template<typename T>
void uf::Time<T>::set(uf::Time<T>::time_t t) { 
	this->m_time = t;
}
template<typename T>
void uf::Time<T>::set(uf::Time<T>::time_t t, uf::Time<T>::exp_t b) { 
	this->m_time = t;
	this->m_exp = b;
}
template<typename T>
inline void uf::Time<T>::fromSeconds(uf::Time<T>::time_t t) { 
	this->set(t, uf::Time<T>::seconds );
}
template<typename T>
inline void uf::Time<T>::fromMilliseconds(uf::Time<T>::time_t t) { 
	this->set(t, uf::Time<T>::milliseconds );
}
template<typename T>
inline void uf::Time<T>::fromMicroseconds(uf::Time<T>::time_t t) { 
	this->set(t, uf::Time<T>::microseconds );
}
template<typename T>
inline void uf::Time<T>::fromNanoseconds(uf::Time<T>::time_t t) { 
	this->set(t, uf::Time<T>::nanoseconds );
}

template<typename T>
typename uf::Time<T>::time_t uf::Time<T>::get() const { 
	return this->m_time;
}
template<typename T>
typename uf::Time<T>::exp_t uf::Time<T>::getBase() const { 
	return this->m_exp;
}

template<typename T>
typename uf::Time<T>::time_t uf::Time<T>::asBase(uf::Time<T>::exp_t base) const { 
	return this->m_time * pow((int) 10, this->m_exp - base);
}
template<typename T>
inline typename uf::Time<T>::time_t uf::Time<T>::asSeconds() const {
	return this->asBase( uf::Time<T>::seconds );
}
template<typename T>
inline typename uf::Time<T>::time_t uf::Time<T>::asMilliseconds() const {
	return this->asBase( uf::Time<T>::milliseconds );
}
template<typename T>
inline typename uf::Time<T>::time_t uf::Time<T>::asMicroseconds() const {
	return this->asBase( uf::Time<T>::microseconds );
}
template<typename T>
inline typename uf::Time<T>::time_t uf::Time<T>::asNanoseconds() const {
	return this->asBase( uf::Time<T>::nanoseconds );
}
template<typename T>
float uf::Time<T>::asFloat() const {
	return this->m_time * pow(10.0f, this->m_exp);
}
template<typename T>
double uf::Time<T>::asDouble() const {
	return this->m_time * pow(10.0, this->m_exp);
}

template<typename T>
uf::Time<T>& uf::Time<T>::operator=( const uf::Time<T>::time_t& t ) { 
	this->m_time = t;
	return *this;
}
template<typename T>
uf::Time<T>::operator double() const {
	return this->asDouble();
}
template<typename T>
uf::Time<T>::operator float() const {
	return this->asFloat();
}
template<typename T>
bool uf::Time<T>::operator>( const uf::Time<T>::time_t& t ) const {
	return this->m_time > t;
}
template<typename T>
bool uf::Time<T>::operator>=( const uf::Time<T>::time_t& t ) const {
	return this->m_time >= t;
}
template<typename T>
bool uf::Time<T>::operator<( const uf::Time<T>::time_t& t ) const {
	return this->m_time < t;
}
template<typename T>
bool uf::Time<T>::operator<=( const uf::Time<T>::time_t& t ) const {
	return this->m_time <= t;
}
template<typename T>
bool uf::Time<T>::operator==( const uf::Time<T>::time_t& t ) const {
	return this->m_time == t;
}
template<typename T>
bool uf::Time<T>::operator>( const uf::Time<T>& t ) const {
	return this->m_time > t.m_time;
}
template<typename T>
bool uf::Time<T>::operator>=( const uf::Time<T>& t ) const {
	return this->m_time >= t.m_time;
}
template<typename T>
bool uf::Time<T>::operator<( const uf::Time<T>& t ) const {
	return this->m_time < t.m_time;
}
template<typename T>
bool uf::Time<T>::operator<=( const uf::Time<T>& t ) const {
	return this->m_time <= t.m_time;
}
template<typename T>
bool uf::Time<T>::operator==( const uf::Time<T>& t ) const {
	return this->m_time == t.m_time;
}
template<typename T>
uf::Time<T> uf::Time<T>::operator-( const uf::Time<T>& t ) const {
	return uf::Time<T>(this->m_time - t.m_time);
}
template<typename T>
uf::Time<T> uf::Time<T>::operator+( const uf::Time<T>& t ) const {
	return uf::Time<T>(this->m_time + t.m_time);
}