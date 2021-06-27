template<typename Arg>
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<void(Arg)>& callback ) {
	typedef typename std::remove_reference<Arg>::type Argument;
	return addHook(name, [=]( const pod::Hook::userdata_t& userdata ){
		pod::Hook::userdata_t ret;
		
		const Argument& payload = userdata.is<Argument>() ? userdata.get<Argument>() : Argument{};
		Argument& unconst_payload = const_cast<Argument&>(payload);
		callback( unconst_payload );
		
		return ret;
	}, pod::Hook::Type{typeid(Argument).hash_code(), sizeof(Argument)});
}
template<typename R, typename Arg>
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<R(Arg)>& callback ) {
	typedef typename std::remove_reference<Arg>::type Argument;
	return addHook(name, [=]( const pod::Hook::userdata_t& userdata ){
		pod::Hook::userdata_t ret;
		const Argument& payload = userdata.is<Argument>() ? userdata.get<Argument>() : Argument{};
		Argument& unconst_payload = const_cast<Argument&>(payload);
		R res = callback( unconst_payload );
		ret.create<R>(res);
		return ret;
	}, pod::Hook::Type{typeid(Argument).hash_code(), sizeof(Argument)});
}

template<typename Function>
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const Function& lambda ) {
	typedef function_traits<Function> Traits;
	typename Traits::type function = lambda;
	return addHook(name, function);
}
template<typename T>
uf::Hooks::return_t uf::Hooks::call( const uf::Hooks::name_t& name, const T& p ) {
	pod::Hook::userdata_t payload;
	payload.create<T>(p);
	return call( name, payload );
}