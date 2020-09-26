template<typename T>
T* uf::instantiator::alloc() {
	return (T*) alloc( sizeof(T) );
//	return new T;
}
template<typename T>
void uf::instantiator::add( const std::string& name ) {
	if ( !names ) names = new std::unordered_map<std::type_index, std::string>;
	if ( !map ) map = new std::unordered_map<std::string, function_t>;

	auto& names = *uf::instantiator::names;
	auto& map = *uf::instantiator::map;

	names[std::type_index(typeid(T))] = name;
	map[name] = _instantiate<T>;

	std::cout << "Registered instantiation for " << name << std::endl;
}
template<typename T>
T& uf::instantiator::instantiate( size_t size ) {
	T* entity = alloc<T>();
	::new (entity) T();
	return *entity;
}
template<typename T>
T& uf::instantiator::instantiate() {
	return instantiate<T>( sizeof(T) );
}
template<typename T>
T* uf::instantiator::_instantiate() {
	return &instantiate<T>( sizeof(T) );
}