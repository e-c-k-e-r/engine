template<typename T>
uf::ThreadUnique<T>::~ThreadUnique() {
	for ( auto& pair : m_mutex_container ) {
		if ( !pair.second ) continue;
		delete pair.second;
		pair.second = NULL;
	}
}

template<typename T>
bool uf::ThreadUnique<T>::has( id_t id ) const {
	return m_container.count(id) > 0;
}
template<typename T>
T& uf::ThreadUnique<T>::get( id_t id ) {
	return m_container[id];
}
template<typename T>
void uf::ThreadUnique<T>::lockMutex( id_t id ) {
	if ( m_mutex_container.count(id) == 0 ) m_mutex_container[id] = new std::mutex;
	m_mutex_container[id]->lock();
}
template<typename T>
bool uf::ThreadUnique<T>::tryMutex( id_t id ) {
	if ( m_mutex_container.count(id) == 0 ) m_mutex_container[id] = new std::mutex;
	return m_mutex_container[id]->try_lock();
}
template<typename T>
void uf::ThreadUnique<T>::unlockMutex( id_t id ) {
	if ( m_mutex_container.count(id) == 0 ) m_mutex_container[id] = new std::mutex;
	m_mutex_container[id]->unlock();
}
template<typename T>
std::lock_guard<std::mutex> uf::ThreadUnique<T>::guardMutex( id_t id ) {
	if ( m_mutex_container.count(id) == 0 ) m_mutex_container[id] = new std::mutex;
	return std::lock_guard<std::mutex>(*m_mutex_container[id]);
}
template<typename T>
void uf::ThreadUnique<T>::cleanup( id_t id ) {
	for ( auto it = m_container.begin(); it != m_container.end(); ) {
		if ( it->first == id ) ++it;
		else it = m_container.erase(it);
	}
	for ( auto it = m_mutex_container.begin(); it != m_mutex_container.end(); ) {
		if ( it->first == id ) ++it;
		else {
			delete it->second;
			it = m_mutex_container.erase(it);
		}
	}
}
template<typename T>
typename uf::ThreadUnique<T>::container_t& uf::ThreadUnique<T>::container() {
	return m_container;
}