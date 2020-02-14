template<typename T> T& uf::Entity::as() {
	return *((T*) this);
}
template<typename T> const T& uf::Entity::as() const {
	return *((T*) this);
}
template<typename T> T& uf::Entity::getParent() {
//	static T null;
//	return this->m_parent ? *(T*)this->m_parent : null;
	if ( this->hasParent() ) return *(T*) this->m_parent;
	return uf::Entity::null.as<T>();
}
template<typename T> T& uf::Entity::getRootParent() {
//	static T null;
	uf::Entity* last = this;
	uf::Entity* pointer = this->m_parent;
	while ( pointer != NULL ) {
		last = pointer;
		pointer = pointer->m_parent;
	}
	return *(T*) last;
}
template<typename T> const T& uf::Entity::getParent() const {
//	static const T null;
//	return this->m_parent ? *(T*)this->m_parent : null;
	if ( this->hasParent() ) return *(const T*) this->m_parent;
	return uf::Entity::null.as<T>();
}
template<typename T> const T& uf::Entity::getRootParent() const {
//	static const T null;
	const uf::Entity* last = this;
	const uf::Entity* pointer = this->m_parent;
	while ( pointer != NULL ) {
		last = pointer;
		pointer = pointer->m_parent;
	}
	return *(const T*) last;
}
/*
template<typename T> void uf::Entity::initialize() {
	static_cast<T*>(this)->initialize();
	this->initialize();
}
template<typename T> void uf::Entity::destroy() {
	static_cast<T*>(this)->destroy();
	this->destroy();
}
template<typename T> void uf::Entity::tick() {
	static_cast<T*>(this)->tick();
	this->tick();
}
template<typename T> void uf::Entity::render() {
	static_cast<T*>(this)->render();
	this->render();
}
*/