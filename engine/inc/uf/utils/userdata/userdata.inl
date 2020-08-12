#include <cstdlib> // malloc, free

// Allows copy via assignment!
template<typename T>
pod::Userdata* uf::userdata::create( const T& data ) {
	void* pointer = operator new( sizeof(pod::Userdata) + sizeof(uint8_t) * (sizeof data) );
	pod::Userdata* userdata = (pod::Userdata*) pointer;
	userdata->len = sizeof data;
	//memcpy( userdata->data, &data, sizeof data );
	//new (userdata->data) T(data);
	union {
		uint8_t* from;
		T* to;
	} static kludge;
	kludge.from = userdata->data;
	new (kludge.to) T(data);
	return userdata;
/*
	std::size_t len = sizeof data; 													// get size of data
//	void* pointer = malloc( sizeof(pod::Userdata) + sizeof(uint8_t) * len ); 		// allocate data for the userdata struct, and then some
	void* pointer = operator new( sizeof(pod::Userdata) + sizeof(uint8_t) * (len) ); 	// allocate data for the userdata struct, and then some
	pod::Userdata* userdata = (pod::Userdata*) pointer;
	userdata->len = len; 															// don't forget to store its data's length!
	// Allows warningless conversion from placeholder storage type to userdata type
	union {
		uint8_t* from;
		T* to;
	} static kludge;
	kludge.from = userdata->data;
	new (kludge.to) T(data); 														// copy via placement new w/ copy constructor
	return userdata; 																// return address of userdata
*/
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
	} cast;
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
	} cast;
	cast.original = this->m_pod->data;
	return *cast.casted;
}
// Easy way to get the userdata as a const-reference
template<typename T>
const T& uf::Userdata::get() const {
	union {
		uint8_t* original;
		const T* casted;
	} cast;
	cast.original = this->m_pod->data;
	return *cast.casted;
}