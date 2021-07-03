/*
template<typename T>
typename uf::Allocator<T>::value_type* uf::Allocator<T>::allocate( size_t n ) {
	return static_cast<typename uf::Allocator<T>::value_type*>( uf::allocator::allocate( n * sizeof(T) ) );
}
template<typename T>
void uf::Allocator<T>::deallocate( typename uf::Allocator<T>::value_type* p, size_t size ) noexcept {
	return uf::allocator::deallocate( (void*) p, size );
}

template<typename T>
typename uf::Mallocator<T>::value_type* uf::Mallocator<T>::allocate( size_t n ) {
	return static_cast<typename uf::Mallocator<T>::value_type*>( uf::allocator::malloc_m( n * sizeof(T) ) );
}
template<typename T>
void uf::Mallocator<T>::deallocate( typename uf::Mallocator<T>::value_type* p, size_t size ) noexcept {
	uf::allocator::free_m( p );
}
*/