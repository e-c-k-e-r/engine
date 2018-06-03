#include <uf/utils/hook/hook.h>
#include <uf/utils/io/iostream.h> 	// uf::iostream
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
void uf::HookHandler::call( const Readable::name_t& name ) {
	if ( !this->exists(name) ) return;
	this->call(name, Readable::argument_t());
	this->call(name, Optimal::argument_t());
}
// Calls a hook in readable format
void uf::HookHandler::call( const Readable::name_t& name, const Readable::argument_t& argument ) {
	if ( !this->exists(name) ) return;
	if ( !this->isReadable(name) ) {
		if ( !this->isAliasToReadable(name) ) return;
		auto& aliases = this->m_readable.aliases.at(name);
		for ( auto& alias : aliases ) {
			this->call( alias.name, (argument != "" ? argument : alias.argument) );
		}
		return;
	}
	
	auto& hooks = this->m_readable.hooks.at(name);
	for ( auto& hook : hooks ) {
		hook( argument );
	/*
		try {
			hook( argument );
		} catch ( std::exception& e ) {
			uf::iostream 	<< "ERROR: Exception thrown while calling hook `" << name << "`!" << "\n"
							<< "\twhat(): " << e.what() << "\n";
		//	throw;
		} catch ( bool handled ) {
			if ( !handled ) throw;
		} catch ( ... ) {
			uf::iostream 	<< "ERROR: Exception thrown while calling hook `" << name << "`!" << "\n"
							<< "\twhat(): " << "???" << "\n";
		//	throw;
		}
	*/
	}
}
// Calls a hook in optimal format
void uf::HookHandler::call( const Optimal::name_t& name, const Optimal::argument_t& argument ) {
	if ( !this->exists(name) ) return;
	if ( !this->isOptimal(name) ) {
		if ( !this->isAliasToOptimal(name) ) return;
		auto& aliases = this->m_optimal.aliases.at(name);
		for ( auto& alias : aliases ) {
			this->call( alias.name, (argument ? argument : alias.argument) );
		}
		return;
	}
	
	auto& hooks = this->m_optimal.hooks.at(name);
	for ( auto& hook : hooks ) {
		hook( argument );
	/*
		try {
			hook( argument );
		} catch ( std::exception& e ) {
			uf::iostream 	<< "ERROR: Exception thrown while calling hook `" << name << "`!" << "\n"
							<< "\twhat(): " << e.what() << "\n";
		//	throw;
		} catch ( bool handled ) {
			if ( !handled ) throw;
		} catch ( ... ) {
			uf::iostream 	<< "ERROR: Exception thrown while calling hook `" << name << "`!" << "\n"
							<< "\twhat(): " << "???" << "\n";
		//	throw;
		}
	*/
	}
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