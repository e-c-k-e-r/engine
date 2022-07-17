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

	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	metadata.hooks.bound[parsed].emplace_back(id);

	return id;
}
template<typename T>
uf::Hooks::return_t uf::Object::callHook( const uf::stl::string& name, const T& p ) {
	pod::Hook::userdata_t payload;
	payload.create<T>(p);

	return uf::hooks.call( this->formatHookName( name ), payload );
}

template<typename T>
void uf::Object::queueHook( const uf::stl::string& name, const T& p, float d ) {
//	if ( !uf::Object::timer.running() ) uf::Object::timer.start();
//	double start = uf::Object::timer.elapsed().asDouble();

	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& queue = metadata.hooks.queue.emplace_back(uf::ObjectBehavior::Metadata::Queued{
		.name = name,
		.timeout = uf::time::current + d,
		.type = 1,
	});
	queue.userdata.create<T>(p);
}