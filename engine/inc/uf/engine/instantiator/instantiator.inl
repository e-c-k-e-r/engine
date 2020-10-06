template<typename C>
pod::NamedTypes<C>::type_t pod::NamedTypes<C>::getType( const std::string& name ) {
	for ( auto pair : names ) if ( pair.second == name ) return pair.first;
	return getType<void>();
}
template<typename C>
template<typename T> pod::NamedTypes<C>::type_t pod::NamedTypes<C>::getType() {
	return std::type_index(typeid(T));
}
template<typename C>
template<typename T> std::string pod::NamedTypes<C>::getName() {
	return names[getType<T>()];	
}
template<typename C>
bool pod::NamedTypes<C>::has( const std::string& name, bool useNames ) {
	return useNames ? names.count(getType(name)) > 0 : map.count(name) > 0;
}template<typename C>
template<typename T> bool pod::NamedTypes<C>::has( bool useNames ) {
	return useNames ? names.count(getType<T>()) > 0 : map.count(getName<T>()) > 0;
}
template<typename C>
template<typename T> void pod::NamedTypes<C>::add( const std::string& name, const C& c ) {
	names[getType<T>()] = name;
	map[name] = c;
}
template<typename C>
C& pod::NamedTypes<C>::get( const std::string& name ) {
	return map[name];
}
template<typename C>
template<typename T>
C& pod::NamedTypes<C>::get() {
	return map[getName<T>()];
}

template<typename T>
T* uf::instantiator::alloc() {
	return (T*) alloc( sizeof(T) );
}

template<typename T> void uf::instantiator::registerObject( const std::string& name ) {
	if ( !objects ) objects = new pod::NamedTypes<pod::Instantiator>;
	auto& container = *uf::instantiator::objects;
	container.add<T>(name, {
		.function = _instantiate<T>
	});

	std::cout << "Registered instantiation for " << name << std::endl;
}
template<typename T> void uf::instantiator::registerBehavior( const std::string& name ) {
	if ( !behaviors ) behaviors = new pod::NamedTypes<pod::Behavior>;
	auto& container = *uf::instantiator::behaviors;
	container.add<T>(name, pod::Behavior{
		.type = uf::Behaviors::getType<T>(),
		.initialize = T::initialize,
		.tick = T::tick,
		.render = T::render,
		.destroy = T::destroy,
	});

	std::cout << "Registered behavior for " << name << std::endl;
}

template<typename T> void uf::instantiator::registerBinding( const std::string& name ) {
	if ( !objects ) objects = new pod::NamedTypes<pod::Instantiator>;
	auto& container = *uf::instantiator::objects;
	auto& instantiator = container.get<T>();
	instantiator.behaviors.emplace_back(name);
	
	std::cout << "Registered binding for " << name << std::endl;
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
		auto& behavior = uf::instantiator::behaviors->get( name );
		entity.addBehavior(behavior);
	}
}

template<typename T>
void uf::instantiator::unbind( uf::Entity& entity ) {
	auto& instantiator = uf::instantiator::objects->get<T>();
	for ( auto& name : instantiator.behaviors ) {
		auto& behavior = uf::instantiator::behaviors->get( name );
		entity.removeBehavior(behavior);
	}
}
/*
template<typename T>
T* uf::instantiator::alloc() {
	return (T*) alloc( sizeof(T) );
}

template<typename T>
std::string uf::instantiator::getName() {
	auto type = getType<T>();
	auto& names = *uf::instantiator::names;
	return names[type];
}

template<typename T>
uf::instantiator::type_t uf::instantiator::getType() {
	return std::type_index(typeid(T));
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
void uf::instantiator::add( const std::string& name ) {
	if ( !map ) map = new std::unordered_map<std::string, pod::Instantiator>;
	if ( !names ) names = new std::unordered_map<uf::instantiator::type_t, std::string>;

	auto& map = *uf::instantiator::map;
	auto& names = *uf::instantiator::names;

	auto type = getType<T>();
	names[type] = name;
	map[name] = {
		.function = _instantiate<T>
	};

	std::cout << "Registered instantiation for " << name << std::endl;
}
template<typename T>
void uf::instantiator::add( const pod::Behavior& behavior ) {
	if ( !registered<T>() ) return;
	auto type = getType<T>();
	auto& map = *uf::instantiator::map;
	auto& instantiator = map[getName<T>()];
	instantiator.behaviors.emplace_back(behavior);
}
template<typename T, typename U>
void uf::instantiator::add() {
	return uf::instantiator::add<T>(pod::Behavior{
		.type = uf::Behaviors::getType<U>(),
		.initialize = U::initialize,
		.tick = U::tick,
		.render = U::render,
		.destroy = U::destroy,
	});
}

template<typename T>
bool uf::instantiator::registered() {
	return uf::instantiator::map->count(getName<T>()) > 0;
}

template<typename T>
void uf::instantiator::bind( T& object ) {
	if ( !registered<T>() ) return;
	auto& map = *uf::instantiator::map;
	auto& instantiator = map[getName<T>()];
	for ( auto& behavior : instantiator.behaviors ) {
		object.addBehavior(behavior);
	}
}
*/