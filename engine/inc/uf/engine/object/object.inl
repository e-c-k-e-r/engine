template<typename T>
T uf::Object::loadChild( const uf::Serializer& json, bool initialize ) {
	// T is size_t
	if ( typeid(T) == typeid(std::size_t) ) return this->loadChildUid(json, initialize);
	// T is pointer
	if ( std::is_pointer<T>::value ) return this->loadChildPointer(json, initialize);
	// T is reference
	return this->loadChild(json, initialize);
}
template<typename T>
T uf::Object::loadChild( const std::string& filename, bool initialize ) {
	// T is size_t
	if ( typeid(T) == typeid(std::size_t) ) return this->loadChildUid(filename, initialize);
	// T is pointer
	if ( std::is_pointer<T>::value ) return this->loadChildPointer(filename, initialize);
	// T is reference
	return this->loadChild(filename, initialize);
}