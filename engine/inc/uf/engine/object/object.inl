template<typename T>
T uf::Object::loadChild( const uf::Serializer& json, bool initialize ) {
	// T is size_t
	if ( TYPE(T) == TYPE(size_t) ) return this->loadChildUid(json, initialize);
	// T is pointer
	if ( std::is_pointer<T>::value ) return this->loadChildPointer(json, initialize);
	// T is reference
	return this->loadChild(json, initialize);
}
template<typename T>
T uf::Object::loadChild( const uf::stl::string& filename, bool initialize ) {
	// T is size_t
	if ( TYPE(T) == TYPE(size_t) ) return this->loadChildUid(filename, initialize);
	// T is pointer
	if ( std::is_pointer<T>::value ) return this->loadChildPointer(filename, initialize);
	// T is reference
	return this->loadChild(filename, initialize);
}
template<typename T>
size_t uf::Object::addHook( const uf::stl::string& name, T callback ) {
	uf::stl::string parsed = this->formatHookName( name );
	std::size_t id = uf::hooks.addHook( parsed, callback );
#if UF_ENTITY_METADATA_USE_JSON
	auto& metadata = this->getComponent<uf::Serializer>();
	metadata["system"]["hooks"]["bound"][parsed].emplace_back(id);
#else
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	metadata.hooks.bound[parsed].emplace_back(id);
#endif
	return id;
}
template<typename T>
uf::Hooks::return_t uf::Object::callHook( const uf::stl::string& name, const T& p ) {
	uf::Userdata payload;
	payload.create<T>(p);
	return uf::hooks.call( this->formatHookName( name ), payload );
}