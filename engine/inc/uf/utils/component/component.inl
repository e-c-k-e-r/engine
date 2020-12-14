template<typename T> pod::Component::id_t uf::component::type() {
	return typeid(T).hash_code();
}
template<typename T> bool uf::component::is( const pod::Component& component ) {
	return uf::component::type<T>() == component.id;
}
//
//
/*
template<typename T>
bool uf::Component::hasAlias() const {
	return this->m_aliases.count(uf::component::type<T>()) != 0;
}
template<typename T, typename U>
bool uf::Component::addAlias() {
	if ( this->hasAlias<T>() ) return false;
	this->m_aliases[uf::component::type<T>()] = this->getType<U>();
	return true;
}
*/
template<typename T>
pod::Component::id_t uf::Component::getType() const {
//	if ( this->hasAlias<T>() ) return this->m_aliases.at(uf::component::type<T>());
	return uf::component::type<T>();
}

template<typename T>
bool uf::Component::hasComponent() const {
	return !this->m_container.empty() && this->m_container.count(this->getType<T>()) != 0;
}
template<typename T>
pod::Component* uf::Component::getRawComponentPointer() {
	if ( !this->hasComponent<T>() ) { if ( this->m_addOn404 ) this->addComponent<T>(); else return NULL; }
	pod::Component::id_t id = this->getType<T>();
	pod::Component& component = this->m_container[id];
	return &component;
}
template<typename T>
const pod::Component* uf::Component::getRawComponentPointer() const {
	if ( !this->hasComponent<T>() ) return NULL;
	pod::Component::id_t id = this->getType<T>();
	const pod::Component& component = this->m_container.at(id);
	return &component;
}
template<typename T>
T* uf::Component::getComponentPointer() {
	T* pointer = NULL;
	if ( !this->hasComponent<T>() ) return ( this->m_addOn404 ) ? &this->addComponent<T>() : pointer;
	pod::Component::id_t id = this->getType<T>();
	pod::Component& component = this->m_container[id];

	pointer = &uf::userdata::get<T>(component.userdata);
	return pointer;
}
template<typename T>
const T* uf::Component::getComponentPointer() const {
	const T* pointer = NULL;
	if ( !this->hasComponent<T>() ) return pointer;
	pod::Component::id_t id = this->getType<T>();
	const pod::Component& component = this->m_container.at(id);

	pointer = &uf::userdata::get<T>(component.userdata);
	return pointer;
}
template<typename T>
T& uf::Component::getComponent() {
	static T null;
	T* pointer = this->getComponentPointer<T>();
	return (pointer) ? *pointer : null;
}
template<typename T>
const T& uf::Component::getComponent() const {
	static T null;
	const T* pointer = this->getComponentPointer<T>();
	return (pointer) ? *pointer : null;
}
template<typename T> T& uf::Component::addComponent( const T& data ) {
	if ( this->hasComponent<T>() ) return this->getComponent<T>();
	
	pod::Component::id_t id = this->getType<T>();
	pod::Component& component = this->m_container[id];
	component.userdata = uf::userdata::create(uf::component::memoryPool, data);
/*
	if ( uf::component::memoryPool.size() > 0 ) {
		component.userdata = uf::userdata::create(uf::component::memoryPool, data);
	} else {
		component.userdata = uf::userdata::create(data);
	}
*/
	component.id = id;

	T* pointer = &uf::userdata::get<T>(component.userdata);
	return *pointer;
}
template<typename T> void uf::Component::deleteComponent() {
	if ( !this->hasComponent<T>() ) return;
	pod::Component::id_t id = this->getType<T>();
	pod::Component& component = this->m_container[id];

	uf::userdata::destroy(uf::component::memoryPool, component.userdata);
/*
	if ( uf::component::memoryPool.size() > 0 ) {
		uf::userdata::destroy(component.userdata);
	} else {
		uf::userdata::destroy(uf::component::memoryPool, component.userdata);
	}
*/
//	uf::userdata::destroy(uf::component::memoryPool, component.userdata);
/*
	if ( component.userdata->pointer ) {
		uf::userdata::destroy(uf::component::memoryPool, component.userdata);
	} else {
		uf::userdata::destroy(component.userdata);
	}
*/
}