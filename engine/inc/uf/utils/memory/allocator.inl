template<typename T>
typename uf::Allocator<T>::value_type* uf::Allocator<T>::allocate( size_t n ) {
// 	return static_cast<typename uf::Allocator<T>::value_type*>(::operator new (n*sizeof(typename uf::Allocator<T>::value_type)));
	return static_cast<typename uf::Allocator<T>::value_type*>( uf::allocator::allocate( n * sizeof(typename uf::Allocator<T>::value_type) ) );
}
template<typename T>
void uf::Allocator<T>::deallocate( typename uf::Allocator<T>::value_type* p, size_t size ) noexcept {
// 	::operator delete(p);
	return uf::allocator::deallocate( (void*) p, size );
}