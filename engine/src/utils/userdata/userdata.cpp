#include <uf/utils/userdata/userdata.h> 	// userdata
#include <cstdlib> 							// malloc, free
#include <cstring> 							// memcpy

// Description: Allows abstract creation of anything
// Use: hook subsystem for passing hook data
/* Todo:
 *	[?] create a singleton class to destroy any remaining userdata
 */

// Constructs a pod::Userdata struct with it's 
pod::Userdata* UF_API uf::userdata::create( std::size_t len, void* data ) {
	if ( !data ) return NULL;
	void* pointer = operator new( sizeof(pod::Userdata) + sizeof(uint8_t) * len ); 	// allocate data for the userdata struct, and then some
//	void* pointer = malloc( sizeof(pod::Userdata) + sizeof(uint8_t) * len ); 		// allocate data for the userdata struct, and then some
	pod::Userdata& userdata = *(pod::Userdata*) pointer; 							// cast to reference
	memcpy( userdata.data, data, len ); 											// copy contents from data to Userdata's data
	userdata.len = len; 															// don't forget to store its data's length!
	return &userdata; 																// return address of userdata
}
void UF_API uf::userdata::destroy( pod::Userdata* userdata ) {
//	free(userdata);
	delete[] userdata;
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
	if ( copy.m_pod ) this->create( copy.m_pod->len, copy.m_pod->data );
}

// 	Creates the POD
pod::Userdata* uf::Userdata::create( std::size_t len, void* data ) {
	this->destroy();
	return this->m_pod = uf::userdata::create( len, data );
}
void uf::Userdata::move( Userdata&& move ) {
//	if ( this->m_pod && move.m_pod ) uf::userdata::move( *this->m_pod, *move.m_pod );
	this->m_pod = move.m_pod;
	move.m_pod = NULL;;
}
void uf::Userdata::copy( const Userdata& copy ) {
//	if ( this->m_pod && copy.m_pod ) uf::userdata::copy( *this->m_pod, *copy.m_pod );
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
	static Userdata::pod_t null;
	return this->m_pod ? *this->m_pod : null;
}
// 	Returns a const-reference of POD
const uf::Userdata::pod_t& uf::Userdata::data() const {
	static Userdata::pod_t null;
	return this->m_pod ? *this->m_pod : null;
}
// 	Validity checks
bool uf::Userdata::initialized() const {
	return this->m_pod;
}
// Overloaded ops
uf::Userdata::operator void*() {
	return this->m_pod ? (void*) this->m_pod->data : NULL;
}
uf::Userdata::operator void*() const {
	return this->m_pod ? (void*) this->m_pod->data : NULL;
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