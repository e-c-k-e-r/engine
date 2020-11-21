template<typename T>
bool uf::ThreadUnique<T>::has( id_t id ) const {
	return m_container.count(id) > 0;
}
template<typename T>
T& uf::ThreadUnique<T>::get( id_t id ) {
	return m_container[id];
}
template<typename T>
typename uf::ThreadUnique<T>::container_t& uf::ThreadUnique<T>::container() {
	return m_container;
}