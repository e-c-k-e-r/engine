#include <uf/utils/userdata/userdata.h> 	// userdata
#include <uf/utils/string/base64.h> 		// base64
#include <cstdlib> 							// malloc, free
#include <cstring> 							// memcpy
#include <iostream> 						

uf::MemoryPool uf::userdata::memoryPool;
bool uf::userdata::autoDestruct = true;
bool uf::userdata::autoValidate = true;
// Constructs a pod::Userdata struct
pod::Userdata* UF_API uf::userdata::create( std::size_t len, void* data ) {
	return uf::userdata::create( uf::userdata::memoryPool, len, data );
}
void UF_API uf::userdata::destroy( pod::Userdata* userdata ) {
	return uf::userdata::destroy( uf::userdata::memoryPool, userdata );
}
pod::Userdata* UF_API uf::userdata::copy( const pod::Userdata* userdata ) {
	return uf::userdata::copy( uf::userdata::memoryPool, userdata );
}

size_t uf::userdata::size( size_t len, size_t padding ) {
	return sizeof(pod::Userdata) - sizeof(uint8_t) + len;
}

//
pod::Userdata* UF_API uf::userdata::create( uf::MemoryPool& requestedMemoryPool, size_t len, void* data ) {
	if ( len <= 0 ) return NULL;
	size_t requestedLen = size( len );
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::memoryPool::global;
	pod::Userdata* userdata = (pod::Userdata*) memoryPool.alloc( requestedLen );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::memoryPool::global.size() > 0 ) memoryPool = &uf::memoryPool::global;
	pod::Userdata* userdata = NULL;
	if ( memoryPool )
		userdata = (pod::Userdata*) memoryPool->alloc( requestedLen );
	else {
		userdata = (pod::Userdata*) uf::allocator::malloc_m( requestedLen ); 	// allocate data for the userdata struct, and then some
	}
#endif
	if ( data ) memcpy( userdata->data, data, len );
	else memset( userdata->data, 0, len );
	userdata->len = len;
	userdata->type = UF_USERDATA_CTTI(void);
	return userdata;
}
void UF_API uf::userdata::destroy( uf::MemoryPool& requestedMemoryPool, pod::Userdata* userdata ) {
#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::memoryPool::global;
	memoryPool.free( userdata, size(userdata->len) );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::memoryPool::global.size() > 0 ) memoryPool = &uf::memoryPool::global;
	
	if ( memoryPool ) memoryPool->free( userdata, size(userdata->len) );
	else uf::allocator::free_m(userdata);
#endif
}
pod::Userdata* UF_API uf::userdata::copy( uf::MemoryPool& requestedMemoryPool, const pod::Userdata* userdata ) {
	if ( !userdata || userdata->len <= 0 ) return NULL;
	auto* copied = uf::userdata::create( userdata->len, const_cast<void*>((const void*) userdata->data[0]) );
	copied->type = userdata->type;
	return copied;
}

uf::stl::string UF_API uf::userdata::toBase64( pod::Userdata* userdata ) {
	return uf::base64::encode( userdata->data, userdata->len );
}
pod::Userdata* UF_API uf::userdata::fromBase64( const uf::stl::string& base64 ) {
	uf::stl::vector<uint8_t> decoded = uf::base64::decode( base64 );
	return uf::userdata::create( decoded.size(), decoded.data() );
}
// 	C-tor

// 	Initializes POD
uf::Userdata::Userdata(std::size_t len, void* data) : m_pod(NULL), autoDestruct(uf::userdata::autoDestruct) {
	if ( len && data ) this->create(len, data);
}

// Initializes from POD
uf::Userdata::Userdata( pod::Userdata* pointer ) : m_pod(pointer), autoDestruct(uf::userdata::autoDestruct) {

}

// Move c-tor
uf::Userdata::Userdata( Userdata&& move ) noexcept :
	m_pod(std::move(move.m_pod)),
	autoDestruct(move.autoDestruct) {
}
// Copy c-tor
uf::Userdata::Userdata( const Userdata& userdata ) {
	autoDestruct = userdata.autoDestruct;
	this->copy( userdata );
}

// 	Creates the POD
pod::Userdata* uf::Userdata::create( std::size_t len, void* data ) {
	if ( len <= 0 ) return NULL;
	this->destroy();
	return this->m_pod = uf::userdata::create( len, data );
}
void uf::Userdata::move( Userdata& moved ) {
//	if ( this->m_pod && move.m_pod ) uf::userdata::move( *this->m_pod, *move.m_pod );
	this->destroy();
	this->m_pod = moved.m_pod;
	moved.m_pod = NULL;
}
void uf::Userdata::move( Userdata&& moved ) {
//	if ( this->m_pod && move.m_pod ) uf::userdata::move( *this->m_pod, *move.m_pod );
	this->destroy();
	this->m_pod = moved.m_pod;
	moved.m_pod = NULL;
}
void uf::Userdata::copy( const Userdata& userdata ) {
	if ( !userdata.m_pod || !userdata.m_pod->len ) return;
	this->destroy();
//	this->m_pod = uf::userdata::copy( userdata.m_pod );
	this->create( userdata.m_pod->len, userdata.m_pod->data );
//	this->m_pod->type = userdata.m_pod->type;
/*
//	if ( this->m_pod && copy.m_pod ) uf::userdata::copy( *this->m_pod, *copy.m_pod );
	if ( !userdata ) return;
	this->destroy();
//	this->m_pod = uf::userdata::copy( userdata.m_pod );
*/
}
// 	D-tor
uf::Userdata::~Userdata() noexcept {
	if ( autoDestruct ) this->destroy();
}
// 	Destroys the POD
void uf::Userdata::destroy() {
	if ( this->m_pod ) uf::userdata::destroy( this->m_pod );
}
// 	POD access
// 	Returns a reference of POD
uf::Userdata::pod_t& uf::Userdata::data() {
	static Userdata::pod_t null = { 0 };
	return this->m_pod ? *this->m_pod : null;
}
// 	Returns a const-reference of POD
const uf::Userdata::pod_t& uf::Userdata::data() const {
	static Userdata::pod_t null = { 0 };
	return this->m_pod ? *this->m_pod : null;
}
size_t uf::Userdata::size() const { return this->m_pod->len; }
UF_USERDATA_CTTI_TYPE uf::Userdata::type() const { return this->m_pod->type; }
// 	Validity checks
bool uf::Userdata::initialized() const {
	return this->m_pod != NULL;
}
// Overloaded ops
uf::Userdata::operator void*() {
	// return this->m_pod ? (void*) this->m_pod->data : NULL;
	if ( !this->m_pod ) return NULL;
	return this->m_pod->data;
}
uf::Userdata::operator void*() const {
	// return this->m_pod ? (void*) this->m_pod->data : NULL;
	if ( !this->m_pod ) return NULL;
	return this->m_pod->data;
}
		
uf::Userdata::operator bool() const {
	return this->initialized();
}

uf::Userdata& uf::Userdata::operator=( pod::Userdata* pointer ) {
	this->m_pod = pointer;
	return *this;
}
uf::Userdata& uf::Userdata::operator=( Userdata&& move ) {
	if ( this->m_pod && move.m_pod ) this->m_pod = move.m_pod, move.m_pod = NULL;
	return *this;
}
uf::Userdata& uf::Userdata::operator=( const Userdata& copy ) {
	if ( this->m_pod && copy.m_pod ) this->copy( copy );
	return *this;
}