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

uf::hashed_string uf::Object::formatHookName( const uf::stl::string_view& n ) {
	constexpr uf::stl::string_view PARENT_UID_SUFFIX = "%P-UID%";
	constexpr uf::stl::string_view UID_SUFFIX = "%UID%";

	if ( n.ends_with(PARENT_UID_SUFFIX) ) {
		size_t uid = this->hasParent() ? this->getParent().getUid() : this->getUid();
		uf::hashed_string hash{uf::stl::string_view(n).substr(0, n.size() - PARENT_UID_SUFFIX.size())};
		return uf::algo::fnv1a( FMT_FORMAT("{}", uid), hash );
	}
	if ( n.ends_with(UID_SUFFIX) ) {
		size_t uid = this->getUid();
		uf::hashed_string hash{uf::stl::string_view(n).substr(0, n.size() - UID_SUFFIX.size())};
		return uf::algo::fnv1a( FMT_FORMAT("{}", uid), hash );
	}

	return n;
}
uf::hashed_string uf::Object::formatHookName( const uf::stl::string_view& n, size_t uid, bool fetch ) {
	if ( fetch ) {
		auto* object = (uf::Object*) uf::Entity::globalFindByUid( uid );
		if ( object ) return object->formatHookName( n );
	}

	constexpr uf::stl::string_view UID_SUFFIX = "%UID%";

	if ( n.ends_with(UID_SUFFIX) ) {
		uf::hashed_string hash = uf::stl::string_view(n).substr(0, n.size() - UID_SUFFIX.size());
		return uf::algo::fnv1a( FMT_FORMAT("{}", uid), hash );
	}

	return n;
}

template<typename T>
size_t uf::Object::addHook( const size_t& name, T callback ) {
	size_t id = uf::hooks.addHook( name, callback );
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	metadata.hooks.bound[name].emplace_back(id);
	return id;
}

template<typename K> inline void uf::Object::queueHook( const K& name, float d ) {
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& queue = metadata.hooks.queue.emplace_back(uf::ObjectBehavior::Metadata::Queued{
		.timeout = uf::time::current + d,
	});
	if constexpr ( std::is_same_v<std::decay_t<K>, size_t> ) {
		queue.hash = name;
	} else {
		queue.name = name;
	}
}

template<typename K, typename V>
void uf::Object::queueHook( const K& name, const V& p, float d ) {
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& queue = metadata.hooks.queue.emplace_back(uf::ObjectBehavior::Metadata::Queued{
		.timeout = uf::time::current + d,
	});
	if constexpr ( std::is_same_v<std::decay_t<K>, size_t> ) {
		queue.hash = name;
	} else {
		queue.name = name;
	}
	if constexpr ( std::is_same_v<std::decay_t<V>, ext::json::Value> ) {
		queue.type = -1;
		queue.json = p;
	} else {
		queue.type = 1;
		queue.userdata.create<V>(p);
	}
}