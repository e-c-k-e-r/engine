#include <cstdlib> // malloc, free

// Allows copy via assignment!
template<typename T>
pod::Userdata* uf::userdata::create( const T& data ) {
	return uf::userdata::create<T>( uf::userdata::memoryPool, data );
}
template<typename T>
pod::Userdata* uf::userdata::create( uf::MemoryPool& requestedMemoryPool, const T& data ) {
	// CTTI information of a T& != T
//	if ( std::is_reference<T>() ) return uf::userdata::create<typename std::remove_reference<T>::type>( requestedMemoryPool, data );
	// Register trait
	uf::userdata::registerTrait<T>();
	return uf::userdata::create( requestedMemoryPool, sizeof(data), (const void*) &data, UF_USERDATA_CTTI(T) );
}
// Easy way to get the userdata as a reference
#include <stdexcept>
template<typename T>
T& uf::userdata::get( pod::Userdata* userdata, bool validate ) {
	if ( validate && !uf::userdata::is<T>( userdata ) ) {
	//	UF_MSG_ERROR("Expecting {" << UF_USERDATA_CTTI(T) << ", " << sizeof(T) << "}, got {" << userdata->type << ", " << userdata->len << "}" );
		UF_EXCEPTION("PointeredUserdata size|type mismatch");
	}
	union {
		void* original;
		T* casted;
	} cast;
	cast.original = userdata->data;
	return *cast.casted;
}
template<typename T>
const T& uf::userdata::get( const pod::Userdata* userdata, bool validate ) {
	if ( validate && !uf::userdata::is<T>( userdata ) ) {
	//	UF_MSG_ERROR("Expecting {" << UF_USERDATA_CTTI(T) << ", " << sizeof(T) << "}, got {" << userdata->type << ", " << userdata->len << "}" );
		UF_EXCEPTION("PointeredUserdata size|type mismatch");
	}
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

template<typename T> void uf::userdata::construct( void* dst, const void* src ) {
	::new (dst) T(*static_cast<const T*>(src));
}
template<typename T> void uf::userdata::destruct( void* p ) {
	static_cast<T*>(p)->~T();
}
template<typename T> const pod::UserdataTraits& uf::userdata::registerTrait() {
	auto& trait = uf::userdata::traits[UF_USERDATA_CTTI(T)];
	trait.name = TYPE_NAME(T);
	trait.constructor = &uf::userdata::construct<T>;
	trait.destructor = &uf::userdata::destruct<T>;
	return trait;
}

//
// No need to cast data to a pointer AND get the data's size!
template<typename T>
pod::Userdata* uf::Userdata::create( const T& data ) {
	this->destroy();
	return this->m_pod = uf::userdata::create<T>(data);
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