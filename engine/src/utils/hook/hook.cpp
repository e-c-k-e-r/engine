#include <uf/utils/hook/hook.h>
#include <uf/utils/io/iostream.h> 	// uf::iostream

size_t uf::Hooks::uids = 0;
uf::Hooks uf::hooks;

size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const pod::Hook::function_t& callback, const pod::Hook::Type& type ) {
	auto& hook = this->m_container[name].emplace_back();
	hook.uid = ++uids;
	hook.type = type;
	hook.callback = callback;

	return hook.uid;
}
void uf::Hooks::removeHook( const uf::Hooks::name_t& name, size_t uid ) {
	auto& container = this->m_container[name];
	for ( auto it = container.begin(); it != container.end(); ++it ) {
		if ( it->uid == uid ) {
			this->m_container[name].erase(it);
			break;
		}
	}
}
void uf::Hooks::removeHooks() {
	this->m_container.clear();
}
/*
uf::Hooks::return_t uf::Hooks::call( const uf::Hooks::name_t& name ) {
	pod::Hook::userdata_t payload{};
	return call( name, payload );
}
*/
uf::Hooks::return_t uf::Hooks::call( const uf::Hooks::name_t& name, const pod::Hook::userdata_t& payload ) {
	auto& container = this->m_container[name];
	uf::stl::vector<pod::Hook::userdata_t> results;
	results.reserve( container.size() );
	
	for ( auto& hook : container ) {
	//	if ( payload.type() != hook.type.hash ) continue;
	//	UF_MSG_DEBUG( name << ": " << payload.type() << " == " << hook.type.hash << "\t\t" << payload.size() << " == " << hook.type.size );
		if ( payload.size() != hook.type.size && hook.type.size > 0 ) continue;

		pod::Hook::userdata_t& unconst_payload = const_cast<pod::Hook::userdata_t&>(payload);
		pod::Hook::userdata_t hookResult = hook.callback(unconst_payload);
		auto& returnResult = results.emplace_back();
		returnResult.move( hookResult );
	}
	
	return results;
}

// specialization: void function
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<void()>& callback ) {
	return addHook( name, [=]( const pod::Hook::userdata_t& userdata ){
		callback();
		pod::Hook::userdata_t ret;
		return ret;
	});
}

bool uf::Hooks::exists( const uf::Hooks::name_t& name ) {
	return this->m_container.count(name) > 0;
}