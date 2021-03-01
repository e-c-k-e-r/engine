#include <uf/utils/userdata/userdata.h> 	// userdata
#include <uf/utils/string/base64.h> 		// base64
#include <cstdlib> 							// malloc, free
#include <cstring> 							// memcpy
#include <iostream> 						

// Constructs a pod::PointeredUserdata struct
pod::PointeredUserdata UF_API uf::pointeredUserdata::create( std::size_t len, void* data ) {
	return uf::pointeredUserdata::create( uf::userdata::memoryPool, len, data );
}
void UF_API uf::pointeredUserdata::destroy( pod::PointeredUserdata& userdata ) {
	return uf::pointeredUserdata::destroy( uf::userdata::memoryPool, userdata );
}
pod::PointeredUserdata UF_API uf::pointeredUserdata::copy( const pod::PointeredUserdata& userdata ) {
	return uf::pointeredUserdata::copy( uf::userdata::memoryPool, userdata );
}

size_t uf::pointeredUserdata::size( size_t len, size_t padding ) {
	// padding = 0;
	// return sizeof(size_t) + len + ( sizeof(uint8_t) * padding );
	return len;
}

//
pod::PointeredUserdata UF_API uf::pointeredUserdata::create( uf::MemoryPool& requestedMemoryPool, std::size_t len, void* data ) {
	if ( len <= 0 ) return {};
	size_t requestedLen = size( len );
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::MemoryPool::global;
	pod::PointeredUserdata userdata = {
		.len = requestedLen,
		.data = memoryPool.alloc( data, requestedLen ),
	};
#else
	uf::MemoryPool* memoryPool = {};
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::MemoryPool::global.size() > 0 ) memoryPool = &uf::MemoryPool::global;
	pod::PointeredUserdata userdata = {};
	if ( memoryPool )
		userdata.data = (uint8_t*) memoryPool->alloc( data, requestedLen );
	else {
		userdata.data = (uint8_t*) operator new( requestedLen ); 	// allocate data for the userdata struct, and then some
		if ( data ) memcpy( userdata.data, data, len );
		else memset( userdata.data, 0, len );
	}
#endif
	userdata.len = len;
	userdata.type = typeid(NULL).hash_code();
	return userdata;
}
void UF_API uf::pointeredUserdata::destroy( uf::MemoryPool& requestedMemoryPool, pod::PointeredUserdata& userdata ) {
#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::MemoryPool::global;
	memoryPool.free( userdata, size(userdata.len) );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::MemoryPool::global.size() > 0 ) memoryPool = &uf::MemoryPool::global;
	
	if ( memoryPool ) memoryPool->free( userdata.data, size(userdata.len) );
	else {
		// free(userdata);
		delete[] userdata.data;
	}
#endif
	userdata.len = 0;
	userdata.data = NULL;
}
pod::PointeredUserdata UF_API uf::pointeredUserdata::copy( uf::MemoryPool& requestedMemoryPool, const pod::PointeredUserdata& userdata ) {
	if ( !userdata.data || userdata.len <= 0 ) return {};
	auto copied = uf::pointeredUserdata::create( userdata.len, const_cast<uint8_t*>(userdata.data) );
#if UF_USERDATA_RTTI
//	copied->type = userdata.type;
#endif
	return copied;
}

std::string UF_API uf::pointeredUserdata::toBase64( pod::PointeredUserdata& userdata ) {
	return uf::base64::encode( userdata.data, userdata.len );
}
pod::PointeredUserdata UF_API uf::pointeredUserdata::fromBase64( const std::string& base64 ) {
	std::vector<uint8_t> decoded = uf::base64::decode( base64 );
	return uf::pointeredUserdata::create( decoded.size(), decoded.data() );
}
// 	C-tor

// 	Initializes POD
uf::PointeredUserdata::PointeredUserdata(std::size_t len, void* data) : m_pod({}), autoDestruct(uf::userdata::autoDestruct) {
	if ( len && data ) this->create(len, data);
}

// Initializes from POD
uf::PointeredUserdata::PointeredUserdata( const pod::PointeredUserdata& pointer ) : m_pod(pointer), autoDestruct(uf::userdata::autoDestruct) {

}

// Move c-tor
uf::PointeredUserdata::PointeredUserdata( PointeredUserdata&& move ) noexcept :
	m_pod(std::move(move.m_pod)),
	autoDestruct(move.autoDestruct) {
}
// Copy c-tor
uf::PointeredUserdata::PointeredUserdata( const PointeredUserdata& userdata ) {
	autoDestruct = userdata.autoDestruct;
	this->copy( userdata );
}

// 	Creates the POD
pod::PointeredUserdata& uf::PointeredUserdata::create( std::size_t len, void* data ) {
	if ( len <= 0 ) return this->m_pod;
	this->destroy();
	return this->m_pod = uf::pointeredUserdata::create( len, data );
}
void uf::PointeredUserdata::move( PointeredUserdata& moved ) {
//	if ( this->m_pod && move.m_pod ) uf::pointeredUserdata::move( *this->m_pod, *move.m_pod );
	this->destroy();
	this->m_pod = moved.m_pod;
	moved.m_pod.len = 0;
	moved.m_pod.data = NULL;
}
void uf::PointeredUserdata::move( PointeredUserdata&& moved ) {
//	if ( this->m_pod && move.m_pod ) uf::pointeredUserdata::move( *this->m_pod, *move.m_pod );
	this->destroy();
	this->m_pod = moved.m_pod;
	moved.m_pod.len = 0;
	moved.m_pod.data = NULL;
}
void uf::PointeredUserdata::copy( const PointeredUserdata& userdata ) {
	if ( !userdata.m_pod.data || !userdata.m_pod.len ) return;
	this->destroy();
//	this->m_pod = uf::pointeredUserdata::copy( userdata.m_pod );
	this->create( userdata.m_pod.len, userdata.m_pod.data );
#if UF_USERDATA_RTTI
//	this->m_pod.type = userdata.m_pod.type;
#endif
/*
//	if ( this->m_pod && copy.m_pod ) uf::pointeredUserdata::copy( *this->m_pod, *copy.m_pod );
	if ( !userdata ) return;
	this->destroy();
//	this->m_pod = uf::pointeredUserdata::copy( userdata.m_pod );
*/
}
// 	D-tor
uf::PointeredUserdata::~PointeredUserdata() noexcept {
	if ( autoDestruct ) this->destroy();
}
// 	Destroys the POD
void uf::PointeredUserdata::destroy() {
	uf::pointeredUserdata::destroy( this->m_pod );
}
// 	POD access
// 	Returns a reference of POD
uf::PointeredUserdata::pod_t& uf::PointeredUserdata::data() {
	return this->m_pod;
}
// 	Returns a const-reference of POD
const uf::PointeredUserdata::pod_t& uf::PointeredUserdata::data() const {
	return this->m_pod;
}
// 	Validity checks
bool uf::PointeredUserdata::initialized() const {
	return this->m_pod.len > 0 && this->m_pod.data;
}
// Overloaded ops
uf::PointeredUserdata::operator void*() {
	// return this->m_pod ? (void*) this->m_pod.data : NULL;
	return this->m_pod.data;
}
uf::PointeredUserdata::operator void*() const {
	// return this->m_pod ? (void*) this->m_pod.data : NULL;
	return this->m_pod.data;
}
		
uf::PointeredUserdata::operator bool() const {
	return this->initialized();
}

uf::PointeredUserdata& uf::PointeredUserdata::operator=( pod::PointeredUserdata pointer ) {
	this->m_pod = pointer;
	return *this;
}
uf::PointeredUserdata& uf::PointeredUserdata::operator=( PointeredUserdata&& move ) {
	if ( this->initialized() && move.initialized() ) {
		this->m_pod = move.m_pod;
		move.m_pod = {};
	}
	return *this;
}
uf::PointeredUserdata& uf::PointeredUserdata::operator=( const PointeredUserdata& copy ) {
	if ( this->initialized() && copy.initialized() ) this->copy( copy );
	return *this;
}