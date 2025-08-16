#include <uf/utils/userdata/userdata.h> 	// userdata
#include <uf/utils/string/base64.h> 		// base64
#include <cstdlib> 							// malloc, free
#include <cstring> 							// memcpy
#include <iostream> 						

// Constructs a pod::PointeredUserdata struct
pod::PointeredUserdata uf::pointeredUserdata::create( size_t len, const void* data, UF_USERDATA_CTTI_TYPE type ) {
	return uf::pointeredUserdata::create( uf::userdata::memoryPool, len, data, type );
}
void uf::pointeredUserdata::destroy( pod::PointeredUserdata& userdata ) {
	return uf::pointeredUserdata::destroy( uf::userdata::memoryPool, userdata );
}
pod::PointeredUserdata uf::pointeredUserdata::copy( const pod::PointeredUserdata& userdata ) {
	return uf::pointeredUserdata::copy( uf::userdata::memoryPool, userdata );
}

size_t uf::pointeredUserdata::size( size_t len, size_t padding ) {
	return len;
}

//
pod::PointeredUserdata uf::pointeredUserdata::create( uf::MemoryPool& requestedMemoryPool, size_t len, const void* data, UF_USERDATA_CTTI_TYPE type ) {
	if ( len <= 0 ) return {};
	size_t requestedLen = size( len );
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::memoryPool::global;
	pod::PointeredUserdata userdata = {
		.len = requestedLen,
		.type = type,
		.data = memoryPool.alloc( requestedLen ),
	};
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::memoryPool::global.size() > 0 ) memoryPool = &uf::memoryPool::global;
	pod::PointeredUserdata userdata = {};
	if ( memoryPool ) userdata.data = memoryPool->alloc( requestedLen );
	else userdata.data = uf::allocator::malloc_m( requestedLen ); // allocate data for the userdata struct, and then some
	userdata.len = requestedLen;
	userdata.type = type;
#endif
	auto& trait = uf::pointeredUserdata::getTrait( userdata.type );
	if ( data && userdata.type != UF_USERDATA_CTTI(void) ) trait.constructor( userdata.data, data );
	else if ( data ) memcpy( userdata.data, data, requestedLen );
	else memset( userdata.data, 0, requestedLen );
	//UF_MSG_DEBUG("Calling create: {}: {}", trait.name, (void*) userdata.data);
	return userdata;
}
void uf::pointeredUserdata::destroy( uf::MemoryPool& requestedMemoryPool, pod::PointeredUserdata& userdata ) {
	if ( !userdata.data ) return;
	auto& trait = uf::pointeredUserdata::getTrait( userdata.type );
	if ( userdata.type != UF_USERDATA_CTTI(void) ) trait.destructor( userdata.data );
	//UF_MSG_DEBUG("Calling destroy: {}: {}", trait.name, (void*) userdata.data);
#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::memoryPool::global;
	memoryPool.free( userdata.data, size(userdata.len) );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::memoryPool::global.size() > 0 ) memoryPool = &uf::memoryPool::global;
	
	if ( memoryPool ) memoryPool->free( userdata.data, size(userdata.len) );
	else uf::allocator::free_m(userdata.data);
#endif
	userdata.len = 0;
	userdata.data = NULL;
	userdata.type = UF_USERDATA_CTTI(void);
}
pod::PointeredUserdata uf::pointeredUserdata::copy( uf::MemoryPool& requestedMemoryPool, const pod::PointeredUserdata& userdata ) {
	if ( !userdata.data || userdata.len <= 0 ) return {};
	pod::PointeredUserdata copied = uf::pointeredUserdata::create( userdata.len, const_cast<void*>(userdata.data) );
	copied.type = userdata.type;
	return copied;
}

const pod::UserdataTraits& uf::pointeredUserdata::getTrait( UF_USERDATA_CTTI_TYPE type ) {
	return uf::userdata::traits[type];
}

uf::stl::string uf::pointeredUserdata::toBase64( pod::PointeredUserdata& userdata ) {
	return uf::base64::encode( static_cast<uint8_t*>(userdata.data), userdata.len );
}
pod::PointeredUserdata uf::pointeredUserdata::fromBase64( const uf::stl::string& base64 ) {
	uf::stl::vector<uint8_t> decoded = uf::base64::decode( base64 );
	return uf::pointeredUserdata::create( decoded.size(), decoded.data() );
}
// 	C-tor

// 	Initializes POD
uf::PointeredUserdata::PointeredUserdata(size_t len, const void* data, UF_USERDATA_CTTI_TYPE type) : m_pod({}), autoDestruct(uf::userdata::autoDestruct) {
	if ( len && data ) this->create(len, data, type);
}

// Initializes from POD
uf::PointeredUserdata::PointeredUserdata( const pod::PointeredUserdata& userdata ) : m_pod({}), autoDestruct(uf::userdata::autoDestruct) {
	this->create( userdata.len, userdata.data, userdata.type );
}

// Move c-tor
uf::PointeredUserdata::PointeredUserdata( PointeredUserdata&& move ) noexcept : m_pod(move.m_pod), autoDestruct(move.autoDestruct) {
	move.m_pod = {};
}
// Copy c-tor
uf::PointeredUserdata::PointeredUserdata( const PointeredUserdata& userdata ) {
	autoDestruct = userdata.autoDestruct;
	this->copy( userdata );
}

// 	Creates the POD
pod::PointeredUserdata& uf::PointeredUserdata::create( size_t len, const void* data, UF_USERDATA_CTTI_TYPE type ) {
	if ( len <= 0 ) return this->m_pod;
	this->destroy();
	return this->m_pod = uf::pointeredUserdata::create( len, data, type );
}
void uf::PointeredUserdata::move( PointeredUserdata& moved ) {
//	if ( this->m_pod && move.m_pod ) uf::pointeredUserdata::move( *this->m_pod, *move.m_pod );
	this->destroy();
	this->m_pod = moved.m_pod;
	moved.m_pod = {};
}
void uf::PointeredUserdata::move( PointeredUserdata&& moved ) {
//	if ( this->m_pod && move.m_pod ) uf::pointeredUserdata::move( *this->m_pod, *move.m_pod );
	this->destroy();
	this->m_pod = moved.m_pod;
	moved.m_pod = {};
}
void uf::PointeredUserdata::copy( const PointeredUserdata& userdata ) {
	if ( !userdata.m_pod.data || !userdata.m_pod.len ) return;
	this->destroy();
	this->create( userdata.m_pod.len, userdata.m_pod.data, userdata.m_pod.type );
}
// 	D-tor
uf::PointeredUserdata::~PointeredUserdata() noexcept {
	if ( autoDestruct ) this->destroy();
}
// 	Destroys the POD
void uf::PointeredUserdata::destroy() {
	uf::pointeredUserdata::destroy( this->m_pod );
	this->m_pod = {};
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
size_t uf::PointeredUserdata::size() const { return this->m_pod.len; }
UF_USERDATA_CTTI_TYPE uf::PointeredUserdata::type() const { return this->m_pod.type; }
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

uf::PointeredUserdata& uf::PointeredUserdata::operator=( const pod::PointeredUserdata& userdata ) {
	this->create( userdata.len, userdata.data, userdata.type );
	return *this;
}
uf::PointeredUserdata& uf::PointeredUserdata::operator=( PointeredUserdata&& move ) {
	if ( this->m_pod.data != move.m_pod.data ) {
		this->destroy();
		this->m_pod = move.m_pod;
		this->autoDestruct = move.autoDestruct;

		move.m_pod = {};
		move.autoDestruct = false;
	}
	return *this;
}
uf::PointeredUserdata& uf::PointeredUserdata::operator=( const PointeredUserdata& copy ) {
	if ( this->initialized() && copy.initialized() ) this->copy( copy );
	return *this;
}