#include <uf/utils/type/type.h>

template<typename T>
T& uf::MemoryPool::alloc( const T& data ) {
	auto allocation = this->allocate( data );
	union {
		uint8_t* from;
		T* to;
	} kludge;
	kludge.from = (uint8_t*) allocation.pointer;
	return *kludge.to;
}
template<typename T>
pod::Allocation uf::MemoryPool::allocate( const T& data ) {
	auto allocation = this->allocate( NULL, sizeof(data) );
	if ( !allocation.pointer ) return allocation;
	union {
		uint8_t* from;
		T* to;
	} kludge;
	kludge.from = (uint8_t*) allocation.pointer;
	::new (kludge.to) T(data);
	return allocation;
}
template<typename T>
bool uf::MemoryPool::exists( const T& data ) {
	if ( std::is_pointer<T>::value ) return this->exists( (void*) data );
	return this->exists( (void*) &data, sizeof(data) );
//	return this->exists( (void*) (std::is_pointer<T>::value ? data : &data), sizeof(data) );
}
template<typename T>
bool uf::MemoryPool::free( const T& data ) {
	if ( std::is_pointer<T>::value ) return this->free( (void*) data );
	return this->free( (void*) &data, sizeof(data) );
//	return this->free( (void*) (std::is_pointer<T>::value ? data : &data), sizeof(data) );
}