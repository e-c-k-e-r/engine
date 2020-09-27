#include <cstdlib> // malloc, free

// Allows copy via assignment!
template<typename T>
pod::Userdata* uf::userdata::create( const T& data ) {
	return uf::userdata::create<T>( uf::userdata::memoryPool, data );
}
template<typename T>
pod::Userdata* uf::userdata::create( uf::MemoryPool& requestedMemoryPool, const T& data ) {
//	uf::MemoryPool& memoryPool = uf::MemoryPool::global.size() > 0 ? uf::MemoryPool::global : requestedMemoryPool;
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::MemoryPool::global;
	pod::Userdata* userdata = (pod::Userdata*) memoryPool.alloc( NULL, sizeof(pod::Userdata) + sizeof(data) );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::MemoryPool::global.size() > 0 ) memoryPool = &uf::MemoryPool::global;
	pod::Userdata* userdata = NULL;
	if ( memoryPool )
		userdata = (pod::Userdata*) memoryPool->alloc( NULL, sizeof(pod::Userdata) + sizeof(data) );
	else
		userdata = (pod::Userdata*) operator new( sizeof(pod::Userdata) + sizeof(data) ); 	// allocate data for the userdata struct, and then some	}
#endif
	userdata->len = sizeof(data);
	union {
		uint8_t* from;
		T* to;
	} static kludge;
	kludge.from = userdata->data;
	::new (kludge.to) T(data);
	return userdata;
}
// Easy way to get the userdata as a reference
#include <stdexcept>
template<typename T>
T& uf::userdata::get( pod::Userdata* userdata, bool validate ) {
	if ( validate && userdata->len != sizeof(T) ) {
		throw std::logic_error("Userdata size mismatch");
	}
	union {
		uint8_t* original;
		T* casted;
	} cast;
	cast.original = userdata->data;
	return *cast.casted;
}
template<typename T>
const T& uf::userdata::get( const pod::Userdata* userdata ) {
	union {
		uint8_t* original;
		const T* casted;
	} static cast;
	cast.original = userdata->data;
	return *cast.casted;
}
// No need to cast data to a pointer AND get the data's size!
template<typename T>
pod::Userdata* uf::Userdata::create( const T& data ) {
	this->destroy();
	return this->m_pod = uf::userdata::create(data);
}
// Easy way to get the userdata as a reference
template<typename T>
T& uf::Userdata::get() {
	union {
		uint8_t* original;
		T* casted;
	} static cast;
	cast.original = this->m_pod->data;
	return *cast.casted;
}
// Easy way to get the userdata as a const-reference
template<typename T>
const T& uf::Userdata::get() const {
	union {
		uint8_t* original;
		const T* casted;
	} static cast;
	cast.original = this->m_pod->data;
	return *cast.casted;
}