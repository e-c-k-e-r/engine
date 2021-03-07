#include <uf/utils/hook/hook.h>
#include <uf/utils/io/iostream.h> 	// uf::iostream

size_t uf::Hooks::uids = 0;
uf::Hooks uf::hooks;

size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const pod::Hook::function_t& callback, size_t type ) {
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
uf::Hooks::return_t uf::Hooks::call( const uf::Hooks::name_t& name, const pod::Hook::userdata_t& payload ) {
	auto& container = this->m_container[name];
	std::vector<pod::Hook::userdata_t> results;
	results.reserve( container.size() );
/*
	if ( name[0] == 'w' ) {
		std::cout << "Calling hook: " << name;
		if ( payload.is<std::string>() ) std::cout << " (string) " << payload.get<std::string>();
		else if ( payload.is<ext::json::Value>() ) std::cout << " (json) " << payload.get<ext::json::Value>();
		else if ( payload.is<uf::Serializer>() ) std::cout << " (serializer) " << payload.get<uf::Serializer>();
		std::cout << std::endl;
	}
*/
	for ( auto& hook : container ) {
		pod::Hook::userdata_t& unconst_payload = const_cast<pod::Hook::userdata_t&>(payload);
		pod::Hook::userdata_t hookResult = hook.callback(unconst_payload);
		auto& returnResult = results.emplace_back();
		returnResult.move( hookResult );
	}
	

	return results;
}
uf::Hooks::return_t uf::Hooks::call( const uf::Hooks::name_t& name, const std::string& s ) {
	pod::Hook::userdata_t payload;
	payload.create<ext::json::Value>();
	auto& value = payload.get<ext::json::Value>();
	value = uf::Serializer(s);
//	payload.create<std::string>( s );
	return call(name, payload);
}
uf::Hooks::return_t uf::Hooks::call( const uf::Hooks::name_t& name, const ext::json::Value& s ) {
	pod::Hook::userdata_t payload;
	payload.create<ext::json::Value>( s );
//	payload.create<std::string>( ext::json::encode( s ) );
	return call(name, payload);
}
uf::Hooks::return_t uf::Hooks::call( const uf::Hooks::name_t& name, const uf::Serializer& s ) {
	pod::Hook::userdata_t payload;
	payload.create<ext::json::Value>( (const ext::json::Value&) s );
//	payload.create<std::string>( s.serialize() );
	return call(name, payload);
}
// specialization: void function
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<void()>& callback ) {
	return addHook( name, [=]( const pod::Hook::userdata_t& userdata ){
		callback();
		pod::Hook::userdata_t ret;
		return ret;
	});
}
/*
// specialization: serialized JSON handling
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<void(const std::string&)>& callback ) {
	return addHook( name, [=]( const pod::Hook::userdata_t& userdata ){
		std::string payload = userdata.is<std::string>() ? userdata.get<std::string>() : "";
		callback( payload );
		pod::Hook::userdata_t ret;
		return ret;
	});
}
// specialization: legacy callback handling
size_t uf::Hooks::addHook( const uf::Hooks::name_t& name, const std::function<std::string(const std::string&)>& callback ) {
	return addHook( name, [=]( const pod::Hook::userdata_t& userdata ){
		std::string payload = userdata.is<std::string>() ? userdata.get<std::string>() : "";
		std::string res = callback( payload );

		pod::Hook::userdata_t ret;
		ret.create<std::string>(res);
		return ret;
	});
}
*/
bool uf::Hooks::exists( const uf::Hooks::name_t& name ) {
	return this->m_container.count(name) > 0;
}

#if 0
uf::HookHandler uf::hooks;
namespace {
	std::size_t hooks = 0;
}
uf::Hooks::~Hooks() {
	this->clear();
}

std::size_t uf::Hooks::addHook( const uf::HookHandler::Readable::name_t& name, const uf::HookHandler::Readable::function_t& callback ) {
	std::size_t id = uf::hooks.addHook( name, callback );
	this->m_container[name].push_back( id );
	return id;
}
std::size_t uf::Hooks::addHook( const uf::HookHandler::Optimal::name_t& name, const uf::HookHandler::Optimal::function_t& callback ) {
	std::size_t id = uf::hooks.addHook( name, callback );
	this->m_container[name].push_back( id );
	return id;
}
void uf::Hooks::clear() {
	for ( uf::Hooks::container_t::iterator it = this->m_container.begin(); it != this->m_container.end(); ++it ) {
	 	const std::string& name = it->first;
	 	const std::vector<std::size_t>& vector = it->second;
		for ( std::size_t id : vector ) uf::hooks.removeHook(name, id);
	}
	this->m_container.clear();
}

uf::HookHandler::HookHandler() :
	m_preferReadable(false)
{
}
// Adds a hook that receives readable data
std::size_t uf::HookHandler::addHook( const Readable::name_t& name, const Readable::function_t& callback ) {
	std::size_t uid = ++::hooks;
	this->m_readable.hooks[name].emplace_back( Readable::hook_t{
		.name = name,
		.callback = callback,
		.uid = uid,
	} );
	return uid;
}
// Adds a hook that receives optimal data
std::size_t uf::HookHandler::addHook( const Optimal::name_t& name, const Optimal::function_t& callback ) {
	std::size_t uid = ++::hooks;
	this->m_optimal.hooks[name].emplace_back( Optimal::hook_t{
		.name = name, 
		.callback = callback,
		.uid = uid,
	} );
	return uid;
}
// Removes a hook
void uf::HookHandler::removeHook( const std::string& name, std::size_t id ) {
	if ( this->isReadable(name) ) {
		auto& hooks = this->m_readable.hooks.at(name);
		for ( auto& hook : hooks ) {
			if ( hook.uid == id ) {
				hooks.erase(std::find(hooks.begin(), hooks.end(), hook));
			}
		}
	}
	if ( this->isOptimal(name) ) {
		auto& hooks = this->m_optimal.hooks.at(name);
		for ( auto& hook : hooks ) {
			if ( hook.uid == id ) {
				hooks.erase(std::find(hooks.begin(), hooks.end(), hook));
			}
		}
	}
}

// Is there a hook bound to a name in either lists?
bool uf::HookHandler::exists( const Readable::name_t& name ) const {
	return this->m_readable.hooks.count(name) > 0 || this->m_optimal.hooks.count(name) > 0;
}

// Is a hook receiving in a readable format...
bool uf::HookHandler::isReadable( const Readable::name_t& name ) const {
	return this->m_readable.hooks.count(name) > 0;
}
// ...or in an optimal one?
bool uf::HookHandler::isOptimal( const Optimal::name_t& name ) const {
	return this->m_optimal.hooks.count(name) > 0;
}
// Swaps between biasing towards readable or not
void uf::HookHandler::shouldPreferReadable(bool bias) {
	this->m_preferReadable = bias;
}
// Do we have a bias towards readable format over optimal? (Used for evaluating hook parameters in dev.)
bool uf::HookHandler::prefersReadable() const {
	return this->m_preferReadable;
}

// 	Aliases
void uf::HookHandler::addAlias( const Readable::alias_t::name_t& name, const Readable::alias_t::name_t& target, const Readable::alias_t::argument_t& argument ) {
	// Can't alias to itself
	if ( name == target ) return;
	this->m_readable.aliases[name].emplace_back( Readable::alias_t{
		.name = name,
		.target = target,
		.argument = argument
	} );
}
void uf::HookHandler::addAlias( const Optimal::alias_t::name_t& name, const Optimal::alias_t::name_t& target, const Optimal::alias_t::argument_t& argument ) {
	// Can't alias to itself
	if ( name == target ) return;
	this->m_optimal.aliases[name].emplace_back( Optimal::alias_t{
		.name = name,
		.target = target,
		.argument = argument
	} );
}
// Is there an alias bound to a hook?
bool uf::HookHandler::isAlias( const Readable::alias_t::name_t& name ) const {
	return this->m_readable.aliases.count(name) > 0 || this->m_optimal.aliases.count(name) > 0;
}
bool uf::HookHandler::isAliasToReadable( const Readable::alias_t::name_t& name ) const {
	return this->m_readable.aliases.count(name) > 0;
}
bool uf::HookHandler::isAliasToOptimal( const Readable::alias_t::name_t& name ) const {
	return this->m_optimal.aliases.count(name) > 0;
}

// Calls a hook in either readable or optimal format (no argument is passed unless it's aliased)
std::vector<uf::HookHandler::Readable::return_t> uf::HookHandler::call( const Readable::name_t& name ) {
	struct {
		std::vector<uf::HookHandler::Readable::return_t> readable;
		std::vector<uf::HookHandler::Optimal::return_t> optimal;
	} returns;
	if ( !this->exists(name) ) return returns.readable;

	returns.readable = this->call(name, Readable::argument_t());
	returns.optimal = this->call(name, Optimal::argument_t());
/*
	if ( typeid(Readable::return_t) == typeid(Optimal::return_t) ) {
		returns.readable.insert( returns.readable.end(), returns.optimal.begin(), returns.optimal.end() );
	}
*/
	return returns.readable;
}
// Calls a hook in readable format
std::vector<uf::HookHandler::Readable::return_t> uf::HookHandler::call( const Readable::name_t& name, const Readable::argument_t& argument ) {
	std::vector<uf::HookHandler::Readable::return_t> returns;
	if ( !this->exists(name) ) return returns;
	if ( !this->isReadable(name) ) {
		if ( !this->isAliasToReadable(name) ) return returns;
		auto& aliases = this->m_readable.aliases.at(name);
		for ( auto& alias : aliases ) {
			auto result = this->call( alias.name, (argument != "" ? argument : alias.argument) );
			returns.insert( returns.end(), result.begin(), result.end() );
		}
		return returns;
	}
	
	auto& hooks = this->m_readable.hooks.at(name);
	for ( auto& hook : hooks ) {
		try{
			auto result = hook( argument );
			returns.push_back(result);
		} catch ( ... ) {
			returns.push_back("");
		}
	}
	return returns;
}
// Calls a hook in optimal format
std::vector<uf::HookHandler::Optimal::return_t> uf::HookHandler::call( const Optimal::name_t& name, const Optimal::argument_t& argument ) {
	std::vector<uf::HookHandler::Optimal::return_t> returns;
	if ( !this->exists(name) ) return returns;
	if ( !this->isOptimal(name) ) {
		if ( !this->isAliasToOptimal(name) ) return returns;
		auto& aliases = this->m_optimal.aliases.at(name);
		for ( auto& alias : aliases ) {
			auto result = this->call( alias.name, (argument ? argument : alias.argument) );
			returns.insert( returns.end(), result.begin(), result.end() );
		}
		return returns;
	}
	
	auto& hooks = this->m_optimal.hooks.at(name);
	for ( auto& hook : hooks ) {
		auto result = hook( argument );
		returns.push_back(result);
	}
	return returns;
}
/*

// uf::Hook
UF_API_CALL uf::Hook::Hook( const uf::Hook::name_t& name, const uf::Hook::function_t& func ) : 
	m_name(name),
	m_function(func)
{

}

const uf::Hook::name_t& UF_API_CALL uf::Hook::getName() const {
	return this->m_name;
}
const uf::Hook::function_t& UF_API_CALL uf::Hook::getFunction() const {
	return this->m_function;
}

uf::Hook::return_t UF_API_CALL uf::Hook::call( const uf::Hook::argument_t& arg ) {
	return this->m_function(arg);
}
// uf::HookAlias
UF_API_CALL uf::HookAlias::HookAlias( const uf::HookAlias::name_t& name, const uf::HookAlias::name_t& target, const uf::HookAlias::argument_t& arg ) : 
	m_name(name),
	m_target(target),
	m_argument(arg)
{

}

const uf::HookAlias::name_t& UF_API_CALL uf::HookAlias::getName() const {
	return this->m_name;
}
const uf::HookAlias::name_t& UF_API_CALL uf::HookAlias::getTarget() const {
	return this->m_target;
}
const uf::HookAlias::argument_t& UF_API_CALL uf::HookAlias::getArgument() const {
	return this->m_argument;
}
// uf::HookHandler
UF_API_CALL uf::HookHandler::HookHandler() {

}

void UF_API_CALL uf::HookHandler::addHook( const uf::Hook::name_t& name, const uf::Hook::function_t& function ) {
	uf::HookHandler::vector_t& hooks = this->m_hooks[name];
	hooks.emplace_back( name, function );
}
const uf::HookHandler::map_t& UF_API_CALL uf::HookHandler::getHooks() const {
	return this->m_hooks;
}
const uf::HookHandler::vector_t& UF_API_CALL uf::HookHandler::getHooks( const uf::Hook::name_t& name ) const {
	static uf::HookHandler::vector_t empty;
	return !this->hookExists(name) ? empty : this->m_hooks.at(name);
}
bool UF_API_CALL uf::HookHandler::hookExists( const uf::Hook::name_t& name ) const {
	return this->m_hooks.count(name) > 0;
}

void UF_API_CALL uf::HookHandler::addAlias( const uf::Hook::name_t& name, const uf::Hook::name_t& target, const uf::Hook::argument_t& arg ) {
	// Can't alias to itself
	if ( name == target ) return;

	uf::HookHandler::alias_vector_t& aliases = this->m_aliases[name];
	aliases.emplace_back( name, target, arg );
}
const uf::HookHandler::alias_map_t& UF_API_CALL uf::HookHandler::getAliases() const {
	return this->m_aliases;
}
const uf::HookHandler::alias_vector_t& UF_API_CALL uf::HookHandler::getAliases( const uf::Hook::name_t& name ) const {
	static uf::HookHandler::alias_vector_t empty;
	return !this->aliasExists(name) ? empty : this->m_aliases.at(name);
}
bool UF_API_CALL uf::HookHandler::aliasExists( const uf::Hook::name_t& name ) const {
	return this->m_aliases.count(name) > 0;
}

uf::HookHandler::return_vector_t UF_API_CALL uf::HookHandler::call( const uf::Hook::name_t& name, const uf::Hook::argument_t& arg ) {
	uf::HookHandler::return_vector_t returns;

	if ( !this->hookExists(name) ) {
		if ( !this->aliasExists(name) ) {
			return returns;
		}
		auto& aliases = this->getAliases(name);
		for ( auto& alias : aliases ) {
			uf::HookHandler::return_vector_t toAppend = this->call( alias.getTarget(), arg );
			returns.insert( returns.end(), toAppend.begin(), toAppend.end() );
		}
		return returns;
	}
	

	auto& hooks = this->m_hooks.at(name);
	for ( auto& hook : hooks ) {
		returns.push_back( hook.call(arg) );
	}

	return returns;
}

*/
#endif