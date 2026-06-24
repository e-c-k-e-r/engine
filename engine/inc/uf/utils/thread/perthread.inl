template<typename T>
bool uf::ThreadUnique<T>::has( id_t id ) const {
	std::lock_guard<std::mutex> lock(*m_mutex);
	return m_container.count(id) > 0;
}
template<typename T>
T& uf::ThreadUnique<T>::get( id_t id ) {
	std::lock_guard<std::mutex> lock(*m_mutex);
	return m_container[id];
}
template<typename T>
uf::ThreadUnique<T>::mutex_type_t uf::ThreadUnique<T>::getMutex( id_t id ) {
 	std::lock_guard<std::mutex> lock(*m_mutex);
	if ( m_mutex_container.count(id) == 0 ) m_mutex_container[id] = std::make_shared<std::mutex>();
	return m_mutex_container[id];
 }

template<typename T>
void uf::ThreadUnique<T>::lockMutex( id_t id ) {
	getMutex( id )->lock();
}
template<typename T>
bool uf::ThreadUnique<T>::tryMutex( id_t id ) {
	return getMutex( id )->try_lock();
}
template<typename T>
void uf::ThreadUnique<T>::unlockMutex( id_t id ) {
	getMutex( id )->unlock();
}
template<typename T>
std::lock_guard<std::mutex> uf::ThreadUnique<T>::guardMutex( id_t id ) {
	return std::lock_guard<std::mutex>(*getMutex( id ));
}
template<typename T>
void uf::ThreadUnique<T>::cleanup( id_t id ) {
	std::lock_guard<std::mutex> lock(*m_mutex);

	for ( auto it = m_container.begin(); it != m_container.end(); ) {
		if ( it->first == id ) it = m_container.erase(it);
		else ++it;
	}
	for ( auto it = m_mutex_container.begin(); it != m_mutex_container.end(); ) {
		if ( it->first == id ) it = m_mutex_container.erase(it);
		else ++it;
	}
}
template<typename T>
typename uf::ThreadUnique<T>::container_t& uf::ThreadUnique<T>::container() {
	return m_container;
}