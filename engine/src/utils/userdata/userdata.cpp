#include <uf/utils/userdata/userdata.h> 	// userdata
#include <uf/utils/string/base64.h> 		// base64
#include <cstdlib> 							// malloc, free
#include <cstring> 							// memcpy
#include <iostream> 						

// Description: Allows abstract creation of anything
// Use: hook subsystem for passing hook data
/* Todo:
 *	[?] create a singleton class to destroy any remaining userdata
 */

uf::MemoryPool uf::userdata::memoryPool;
// Constructs a pod::Userdata struct
pod::Userdata* UF_API uf::userdata::create( std::size_t len, void* data ) {
	return uf::userdata::create( uf::userdata::memoryPool, len, data );
/*
	if ( uf::userdata::memoryPool.size() > 0 ) return uf::userdata::create( uf::userdata::memoryPool, len, data );
#if UF_USERDATA_KLUDGE
	void* pointer = operator new( sizeof(pod::Userdata) + len ); 	// allocate data for the userdata struct, and then some
//	void* pointer = malloc( sizeof(pod::Userdata) + sizeof(uint8_t) * len ); 		// allocate data for the userdata struct, and then some
	pod::Userdata& userdata = *(pod::Userdata*) pointer; 							// cast to reference
	if ( data ) memcpy( userdata.data, data, len ); 											// copy contents from data to Userdata's data
	else memset( userdata.data, 0, len );
	userdata.len = len; 															// don't forget to store its data's length!
	userdata.pointer = NULL; //&userdata.data[0];
	return &userdata; 																// return address of userdata
#else
	pod::Userdata* userdata = new pod::Userdata;
	userdata->len = len;
	userdata->data = (uint8_t*) operator new(len);
	if ( data ) memcpy( userdata->data, data, len );
	else memset( userdata->data, 0, len );
	return userdata;
#endif
*/
}
void UF_API uf::userdata::destroy( pod::Userdata* userdata ) {
	return uf::userdata::destroy( uf::userdata::memoryPool, userdata );
/*
	if ( uf::userdata::memoryPool.size() > 0 ) return uf::userdata::destroy( uf::userdata::memoryPool, userdata );
#if UF_USERDATA_KLUDGE
	if ( userdata->pointer ) return; // allocated with a pool, do not free
	delete[] userdata;
#else
	delete[] userdata->data;
	userdata->len = 0;
	userdata->data = NULL;
	delete userdata;
#endif
*/
}

//
pod::Userdata* UF_API uf::userdata::create( uf::MemoryPool& requestedMemoryPool, std::size_t len, void* data ) {
	if ( len <= 0 ) return NULL;
//	uf::MemoryPool& memoryPool = uf::MemoryPool::global.size() > 0 ? uf::MemoryPool::global : requestedMemoryPool;
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::MemoryPool::global;
	pod::Userdata* userdata = (pod::Userdata*) memoryPool.alloc( data, sizeof(pod::Userdata) + len );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::MemoryPool::global.size() > 0 ) memoryPool = &uf::MemoryPool::global;
	pod::Userdata* userdata = NULL;
	if ( memoryPool )
		userdata = (pod::Userdata*) memoryPool->alloc( data, sizeof(pod::Userdata) + len );
	else {
		userdata = (pod::Userdata*) operator new( sizeof(pod::Userdata) + len ); 	// allocate data for the userdata struct, and then some
		if ( data ) memcpy( userdata->data, data, len );
		else memset( userdata->data, 0, len );
	}
#endif
	userdata->len = len;
	return userdata;
/*
	if ( uf::userdata::memoryPool.size() > 0 ) {
		auto allocation = uf::userdata::memoryPool.allocate( NULL, sizeof(pod::Userdata) + len );
		pod::Userdata* userdata = (pod::Userdata*) allocation.pointer;
		if ( data ) memcpy( userdata->data, data, len );
		else memset( userdata->data, 0, len );
		userdata->len = len;
	#if UF_USERDATA_KLUDGE
		userdata->pointer = NULL;
	#endif
		return userdata;
	} else {
		if ( memoryPool.size() <= 0 ) return uf::userdata::create( len, data );
		pod::Userdata* userdata = new pod::Userdata;
		auto allocation = memoryPool.allocate( data, len );
		userdata->len = allocation.size;
	#if UF_USERDATA_KLUDGE
		userdata->pointer = (uint8_t*) allocation.pointer;
	#else
		userdata->data = (uint8_t*) allocation.pointer;
	#endif
		return userdata;
	}
*/
}
#include <iostream>
void UF_API uf::userdata::destroy( uf::MemoryPool& requestedMemoryPool, pod::Userdata* userdata ) {
//	uf::MemoryPool& memoryPool = uf::MemoryPool::global.size() > 0 ? uf::MemoryPool::global : requestedMemoryPool;

#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::MemoryPool::global;
	memoryPool.free( userdata, sizeof(pod::Userdata) + userdata->len );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::MemoryPool::global.size() > 0 ) memoryPool = &uf::MemoryPool::global;
	
	if ( memoryPool ) memoryPool->free( userdata, sizeof(pod::Userdata) + userdata->len );
	else {
		// free(userdata);
		delete[] userdata;
	}
#endif
/*
	if ( uf::userdata::memoryPool.size() > 0 ) {
		memoryPool.free( userdata, sizeof(pod::Userdata) + userdata->len );
		userdata->len = 0;
	} else {
	#if UF_USERDATA_KLUDGE
		if ( !userdata->pointer ) {
			uf::userdata::destroy( userdata );
			return;
		}
	#endif
		memoryPool.free( userdata->data, userdata->len );
		delete userdata;
	}
*/
}

std::string UF_API uf::userdata::toBase64( pod::Userdata* userdata ) {
	return uf::base64::encode( userdata->data, userdata->len );
}
pod::Userdata* UF_API uf::userdata::fromBase64( const std::string& base64 ) {
	std::vector<uint8_t> decoded = uf::base64::decode( base64 );
	return uf::userdata::create( decoded.size(), decoded.data() );
}
// 	C-tor

// 	Initializes POD
uf::Userdata::Userdata(std::size_t len, void* data) : m_pod(NULL) {
	if ( len && data ) this->create(len, data);
}

// Move c-tor
uf::Userdata::Userdata( Userdata&& move ) :
	m_pod(std::move(move.m_pod))
{

}
// Copy c-tor
uf::Userdata::Userdata( const Userdata& copy ) {
	if ( copy.m_pod && copy.m_pod->len ) this->create( copy.m_pod->len, copy.m_pod->data );
}

// 	Creates the POD
pod::Userdata* uf::Userdata::create( std::size_t len ) {
	if ( len <= 0 ) return NULL;
	this->destroy();
	return this->m_pod = uf::userdata::create( len, NULL );
}
pod::Userdata* uf::Userdata::create( std::size_t len, void* data ) {
	if ( len <= 0 ) return NULL;
	this->destroy();
	return this->m_pod = uf::userdata::create( len, data );
}
void uf::Userdata::move( Userdata&& move ) {
//	if ( this->m_pod && move.m_pod ) uf::userdata::move( *this->m_pod, *move.m_pod );
	std::cout << "MOVE: " << this << " <- " << &move << ", " << this->m_pod << " <- " << move.m_pod << std::endl;
	this->m_pod = move.m_pod;
	move.m_pod = NULL;
}
void uf::Userdata::copy( const Userdata& copy ) {
//	if ( this->m_pod && copy.m_pod ) uf::userdata::copy( *this->m_pod, *copy.m_pod );
	std::cout << "COPY: " << this << " <- " << &copy << ", " << this->m_pod << " <- " << copy.m_pod << std::endl;
	if ( copy.m_pod ) this->create( copy.m_pod->len, copy.m_pod->data );
}
// 	D-tor
uf::Userdata::~Userdata() {
	this->destroy();
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

uf::Userdata& uf::Userdata::operator=( Userdata&& move ) {
	if ( this->m_pod && move.m_pod ) this->m_pod = move.m_pod, move.m_pod = NULL;
	return *this;
}
uf::Userdata& uf::Userdata::operator=( const Userdata& copy ) {
	if ( this->m_pod && copy.m_pod ) this->copy( copy );
	return *this;
}