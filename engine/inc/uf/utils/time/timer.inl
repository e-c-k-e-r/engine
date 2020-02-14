template <typename T>
uf::Timer<T>::Timer(bool running, uf::Timer<T>::exp_t b) :
	m_running(running),
	m_begin(0, b),
	m_end(0, b)
{
	if ( running ) this->start();
}

template <typename T>
void uf::Timer<T>::start( uf::Time<T> offset ) {
	this->m_running = true;
	this->reset();
	this->m_begin = this->m_begin + offset;
}
template <typename T>
void uf::Timer<T>::stop() {
	this->update();
	this->m_running = false;
}
template <typename T>
void uf::Timer<T>::reset() {
	this->m_end = this->m_begin = spec::time.getTime();
}
template <typename T>
void uf::Timer<T>::update() {
	if ( !this->m_running ) return;
	this->m_end = spec::time.getTime();
}
template <typename T>
bool uf::Timer<T>::running() const {
	return this->m_running;
}

template <typename T>
uf::Time<T>& uf::Timer<T>::getStarting() {
	return this->m_begin;
}
template <typename T>
uf::Time<T>& uf::Timer<T>::getEnding() {
	this->update();
	return this->m_end;
}

template <typename T>
uf::Time<T> uf::Timer<T>::elapsed() {
	this->update();
	return this->m_end - this->m_begin;
}
template <typename T>
uf::Time<T> uf::Timer<T>::elapsed() const {
	return this->m_end - this->m_begin;
}
template <typename T>
const uf::Time<T>& uf::Timer<T>::getStarting() const {
	return this->m_begin;
}
template <typename T>
const uf::Time<T>& uf::Timer<T>::getEnding() const {
	return this->m_end;
}