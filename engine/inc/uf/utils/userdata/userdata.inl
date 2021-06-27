#include <cstdlib> // malloc, free

// Allows copy via assignment!
template<typename T>
pod::Userdata* uf::userdata::create( const T& data ) {
	return uf::userdata::create<T>( uf::userdata::memoryPool, data );
}
template<typename T>
pod::Userdata* uf::userdata::create( uf::MemoryPool& requestedMemoryPool, const T& data ) {
//	uf::MemoryPool& memoryPool = uf::MemoryPool::global.size() > 0 ? uf::MemoryPool::global : requestedMemoryPool;
	size_t len = sizeof(data);
	size_t requestedLen = size( len );
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::MemoryPool::global;
	pod::Userdata* userdata = (pod::Userdata*) memoryPool.alloc( NULL, requestedLen );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::MemoryPool::global.size() > 0 ) memoryPool = &uf::MemoryPool::global;
	pod::Userdata* userdata = NULL;
	if ( memoryPool )
		userdata = (pod::Userdata*) memoryPool->alloc( NULL, requestedLen );
	else
		userdata = (pod::Userdata*) operator new( requestedLen ); 	// allocate data for the userdata struct, and then some	}
#endif
#if UF_USERDATA_RTTI
	userdata->type = typeid(T).hash_code();
#endif
	userdata->len = len;
	union {
		uint8_t* from;
		T* to;
	} kludge;
	kludge.from = userdata->data;
	::new (kludge.to) T(data);
	return userdata;
}
// Easy way to get the userdata as a reference
#include <stdexcept>
template<typename T>
T& uf::userdata::get( pod::Userdata* userdata, bool validate ) {
	if ( validate && !uf::userdata::is<T>( userdata ) ) UF_EXCEPTION("PointeredUserdata size|type mismatch: Expecting {" << typeid(T).hash_code() << ", " << sizeof(T) << "}, got {" << userdata->type << ", " << userdata->len << "}" );
	union {
		uint8_t* original;
		T* casted;
	} cast;
	cast.original = userdata->data;
	return *cast.casted;
}
template<typename T>
const T& uf::userdata::get( const pod::Userdata* userdata, bool validate ) {
	if ( validate && !uf::userdata::is<T>( userdata ) ) UF_EXCEPTION("PointeredUserdata size|type mismatch: Expecting {" << typeid(T).hash_code() << ", " << sizeof(T) << "}, got {" << userdata->type << ", " << userdata->len << "}" );
	union {
		const uint8_t* original;
		const T* casted;
	} cast;
	cast.original = userdata->data;
	return *cast.casted;
}

template<typename T>
bool uf::userdata::is( const pod::Userdata* userdata ) {
#if UF_USERDATA_RTTI
	if ( userdata && userdata->type ) return userdata && userdata->type == typeid(T).hash_code() && userdata->len == sizeof(T);
#endif
	return userdata && userdata->len == sizeof(T);
}

// No need to cast data to a pointer AND get the data's size!
template<typename T>
pod::Userdata* uf::Userdata::create( const T& data ) {
	this->destroy();
	return this->m_pod = uf::userdata::create(data);
}
// Easy way to get the userdata as a reference
template<typename T>
T& uf::Userdata::get( bool validate ) {
	return uf::userdata::get<T>( this->m_pod, validate );
}
// Easy way to get the userdata as a const-reference
template<typename T>
const T& uf::Userdata::get( bool validate ) const {
	return uf::userdata::get<T>( this->m_pod, validate );
}

template<typename T>
bool uf::Userdata::is() const {
	return uf::userdata::is<T>( this->m_pod );
}