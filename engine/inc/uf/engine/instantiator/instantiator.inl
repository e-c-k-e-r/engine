// #include <uf/utils/type/type.h>

template<typename C>
typename pod::NamedTypes<C>::type_t pod::NamedTypes<C>::getType( const uf::stl::string& name ) {
	for ( auto pair : names ) if ( pair.second == name ) return pair.first;
	return getType<void>();
}
template<typename C>
template<typename T> typename pod::NamedTypes<C>::type_t pod::NamedTypes<C>::getType() {
	return TYPE(T);
}
template<typename C>
template<typename T> uf::stl::string pod::NamedTypes<C>::getName() {
	return names[getType<T>()];	
}
template<typename C>
bool pod::NamedTypes<C>::has( const uf::stl::string& name, bool useNames ) {
	return useNames ? names.count(getType(name)) > 0 : map.count(name) > 0;
}template<typename C>
template<typename T> bool pod::NamedTypes<C>::has( bool useNames ) {
	return useNames ? names.count(getType<T>()) > 0 : map.count(getName<T>()) > 0;
}
template<typename C>
template<typename T> void pod::NamedTypes<C>::add( const uf::stl::string& name, const C& c ) {
	names[getType<T>()] = name;
	map[name] = c;
}
template<typename C>
C& pod::NamedTypes<C>::get( const uf::stl::string& name ) {
	return map[name];
}
template<typename C>
template<typename T>
C& pod::NamedTypes<C>::get() {
	return map[getName<T>()];
}
template<typename C>
C& pod::NamedTypes<C>::operator[]( const uf::stl::string& name ) {
	return get(name);
}

template<typename T>
T* uf::instantiator::alloc() {
	return (T*) alloc( sizeof(T) );
}

template<typename T> void uf::instantiator::registerObject( const uf::stl::string& name ) {
	if ( !objects ) objects = new pod::NamedTypes<pod::Instantiator>;
	auto& container = *uf::instantiator::objects;
	container.add<T>(name, {
		.function = _instantiate<T>,
		.behaviors = {}
	});

	#if UF_INSTANTIATOR_ANNOUNCE
		UF_MSG_DEBUG("Registered instantiation for {}", name);
	#endif
}
template<typename T> void uf::instantiator::registerBinding( const uf::stl::string& name ) {
	if ( !objects ) objects = new pod::NamedTypes<pod::Instantiator>;
	auto& container = *uf::instantiator::objects;
	auto& instantiator = container.get<T>();
	instantiator.behaviors.emplace_back(name);
	
	#if UF_INSTANTIATOR_ANNOUNCE
		UF_MSG_DEBUG("Registered binding for {}", name);
	#endif
}

template<typename T>
T& uf::instantiator::instantiate() {
	T* entity = alloc<T>();
	::new (entity) T();
	T& object = *entity;
	uf::instantiator::bind<T>( object );
	return object;
}
template<typename T>
T* uf::instantiator::_instantiate() {
	return &instantiate<T>();
}

template<typename T>
void uf::instantiator::bind( uf::Entity& entity ) {
	auto& instantiator = uf::instantiator::objects->get<T>();
	for ( auto& name : instantiator.behaviors ) {
		auto& behavior = (*uf::instantiator::behaviors)[name];
		entity.addBehavior(behavior);
	}
}

template<typename T>
void uf::instantiator::unbind( uf::Entity& entity ) {
	auto& instantiator = uf::instantiator::objects->get<T>();
	for ( auto& name : instantiator.behaviors ) {
		auto& behavior = (*uf::instantiator::behaviors)[name];
		entity.removeBehavior(behavior);
	}
}