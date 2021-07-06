#include <cstdlib> // malloc, free

// Allows copy via assignment!
template<typename T>
pod::Userdata* uf::userdata::create( const T& data ) {
	return uf::userdata::create<T>( uf::userdata::memoryPool, data );
}
template<typename T>
pod::Userdata* uf::userdata::create( uf::MemoryPool& requestedMemoryPool, const T& data ) {
	pod::Userdata* userdata = uf::userdata::create( requestedMemoryPool, sizeof(data), nullptr );
	userdata->type = UF_USERDATA_CTTI(T);
	union {
		void* from;
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
	if ( validate && !uf::userdata::is<T>( userdata ) ) UF_EXCEPTION("PointeredUserdata size|type mismatch"); // : Expecting {" << UF_USERDATA_CTTI(T) << ", " << sizeof(T) << "}, got {" << userdata->type << ", " << userdata->len << "}" );
	union {
		void* original;
		T* casted;
	} cast;
	cast.original = userdata->data;
	return *cast.casted;
}
template<typename T>
const T& uf::userdata::get( const pod::Userdata* userdata, bool validate ) {
	if ( validate && !uf::userdata::is<T>( userdata ) ) UF_EXCEPTION("PointeredUserdata size|type mismatch"); // : Expecting {" << UF_USERDATA_CTTI(T) << ", " << sizeof(T) << "}, got {" << userdata->type << ", " << userdata->len << "}" );
	union {
		const void* original;
		const T* casted;
	} cast;
	cast.original = userdata->data;
	return *cast.casted;
}

template<typename T>
bool uf::userdata::is( const pod::Userdata* userdata ) {
	if ( userdata && userdata->type != UF_USERDATA_CTTI(void) ) return userdata && userdata->type == UF_USERDATA_CTTI(T) && userdata->len == sizeof(T);
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