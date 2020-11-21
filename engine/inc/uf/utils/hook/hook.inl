template<typename Arg>
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<void(Arg)>& callback ) {
	typedef typename std::remove_reference<Arg>::type Argument;
	return addHook(name, [=]( const uf::Userdata& userdata ){
		uf::Userdata ret;
		
		const Argument& payload = userdata.is<Argument>() ? userdata.get<Argument>() : Argument{};
		callback( payload );
		
		return ret;
	}, typeid(callback).hash_code());
}
template<typename R, typename Arg>
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<R(Arg)>& callback ) {
	typedef typename std::remove_reference<Arg>::type Argument;
	return addHook(name, [=]( const uf::Userdata& userdata ){
		uf::Userdata ret;
		const Argument& payload = userdata.is<Argument>() ? userdata.get<Argument>() : Argument{};

		R res = callback( payload );
		ret.create<R>(res);
		return ret;
	}, typeid(callback).hash_code());
}
/*
template<typename R, typename ...Args>
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<R(Args...)>& callback ) {
	typedef function_traits<decltype(callback)> Traits;
	typedef typename std::remove_reference<typename Traits::template Arg<0>::type>::type Argument;
	return addHook(name, [=]( const uf::Userdata& userdata ){
		uf::Userdata ret;
		const Argument& payload = userdata.is<Argument>() ? userdata.get<Argument>() : Argument{};

		R res = callback( payload );
		ret.create<R>(res);

		return ret;
	}, typeid(callback).hash_code());
}
*/
template<typename Function>
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const Function& lambda ) {
	typedef function_traits<Function> Traits;
	typename Traits::type function = lambda;
	return addHook(name, function);
}
template<typename T>
uf::Hooks::return_t uf::Hooks::call( const uf::Hooks::name_t& name, const T& p ) {
	uf::Userdata payload;
	payload.create<T>(p);
	return call( name, payload );
}