#pragma once

#include <uf/config.h>

#include <uf/utils/userdata/userdata.h> 	// uf::Userdata

#include <string> 							// std::string
#include <functional> 						// std::function
#include <vector> 							// std::vector
#include <unordered_map> 					// std::unordered_map

namespace pod {
	template<typename Argument = std::string, typename Return = Argument>
	struct UF_API Hook {
		typedef std::string 							name_t;
		typedef Argument 								argument_t;
		typedef Return 									return_t;
		typedef std::function<Return(const Argument&)> 	function_t;

		Hook<Argument, Return>::name_t 		name;
		Hook<Argument, Return>::function_t 	callback;
		std::size_t uid;

		Return operator()( const Argument& arg = Argument() ) { return callback( arg ); }
		bool operator==( const Hook& hook ) { return this->uid == hook.uid; }
	};
	template<typename Argument = std::string>
	struct UF_API HookAlias {
		typedef typename Hook<Argument>::name_t 		name_t;
		typedef typename Hook<Argument>::argument_t 	argument_t;
		
		typename Hook<Argument>::name_t name;
		typename Hook<Argument>::name_t target;
		typename Hook<Argument>::argument_t argument;
	};
}

namespace uf {
	typedef pod::Hook<> 						ReadableHook;
	typedef pod::Hook<uf::Userdata::pod_t*> 	OptimalHook;
	typedef ReadableHook Hook;

	class UF_API HookHandler {
	public:
	//	template<typename Argument, typename Return = Argument>
		template<typename Hook>
		struct HookType {
			typedef typename Hook::argument_t 						Argument;
			typedef typename Hook::return_t 						Return;
			typedef typename Hook::name_t 							Name;
			typedef Argument 										argument_t;
			typedef Return 											return_t;
			typedef Name 											name_t;

			typedef Hook 											hook_t;
			typedef pod::HookAlias<Argument> 						alias_t;

			typedef typename Hook::function_t 						function_t;
			typedef std::vector<Hook> 								vector_t;
			typedef std::vector<Return> 							returns_t;
			typedef std::unordered_map<Name, vector_t> 				lookup_t;

			typedef std::vector<alias_t> 							aliases_t;
			typedef std::unordered_map<Name, aliases_t> 			alias_lookup_t;

			struct aggregate_t {
				lookup_t hooks;
				alias_lookup_t aliases;
			};
		};

		typedef HookType<uf::ReadableHook> Readable;
		typedef HookType<uf::OptimalHook> Optimal;
	protected:
		Readable::aggregate_t m_readable;
		Optimal::aggregate_t m_optimal;
		bool m_preferReadable;
	public:
		HookHandler();

		std::size_t addHook( const Readable::name_t& name, const Readable::function_t& callback ); 	// Adds a hook that receives readable data
		std::size_t addHook( const Optimal::name_t& name, const Optimal::function_t& callback ); 		// Adds a hook that receives optimal data
		void removeHook( const std::string&, std::size_t );
		
		bool exists( const Readable::name_t& name ) const; 				// Is there a hook bound to a name in either lists?
		
		bool isReadable( const Readable::name_t& name ) const; 			// Is a hook receiving in a readable format...
		bool isOptimal( const Optimal::name_t& name ) const; 			// ...or in an optimal one?
		void shouldPreferReadable(bool bias=true); 						// Swaps between biasing towards readable or not
		bool prefersReadable() const; 									// Do we have a bias towards readable format over optimal? (Used for evaluating hook parameters in dev.)
		// 	Aliases
		void addAlias( const Readable::alias_t::name_t& name, const Readable::alias_t::name_t& target, const Readable::alias_t::argument_t& argument );
		void addAlias( const Optimal::alias_t::name_t& name, const Optimal::alias_t::name_t& target, const Optimal::alias_t::argument_t& argument );
		bool isAlias( const Readable::alias_t::name_t& name ) const; 	// Is there an alias bound to a hook?
		bool isAliasToReadable( const Readable::alias_t::name_t& name ) const; 	// Is there an alias bound to a readable hook?
		bool isAliasToOptimal( const Optimal::alias_t::name_t& name ) const; 	// Is there an alias bound to an optimal hook?

		void call( const Readable::name_t& name );
		void call( const Readable::name_t& name, const Readable::argument_t& argument );
		void call( const Optimal::name_t& name, const Optimal::argument_t& argument );
	};

/*
	class UF_API Hook {
	public:
		typedef std::string name_t;
		
		typedef int return_t;
		typedef std::string argument_t;
		
		typedef std::function<return_t(const argument_t&)> function_t;
	protected:
		uf::Hook::name_t m_name;
		uf::Hook::function_t m_function;
	public:
		UF_API_CALL Hook( const uf::Hook::name_t& name, const uf::Hook::function_t& func );

		const uf::Hook::name_t& UF_API_CALL getName() const;
		const uf::Hook::function_t& UF_API_CALL getFunction() const;

		uf::Hook::return_t UF_API_CALL call( const uf::Hook::argument_t& arg );
	};
	class UF_API HookAlias {
	public:
		typedef uf::Hook::name_t name_t;
		typedef uf::Hook::argument_t argument_t;
	protected:
		uf::HookAlias::name_t m_name;
		uf::HookAlias::name_t m_target;
		uf::HookAlias::argument_t m_argument;
	public:
		UF_API_CALL HookAlias( const uf::HookAlias::name_t& name, const uf::HookAlias::name_t& target, const uf::HookAlias::argument_t& arg );

		const uf::HookAlias::name_t& UF_API_CALL getName() const;
		const uf::HookAlias::name_t& UF_API_CALL getTarget() const;
		const uf::HookAlias::argument_t& UF_API_CALL getArgument() const;
	};
	class UF_API HookHandler {
	public:
		typedef std::vector<uf::Hook> vector_t;
		typedef std::vector<uf::Hook::return_t> return_vector_t;
		typedef std::unordered_map<uf::Hook::name_t, uf::HookHandler::vector_t> map_t;
		
		typedef std::vector<uf::HookAlias> alias_vector_t;
		typedef std::unordered_map<uf::Hook::name_t, uf::HookHandler::alias_vector_t> alias_map_t;
	protected:
		uf::HookHandler::map_t m_hooks;
		uf::HookHandler::alias_map_t m_aliases;
	public:
		UF_API_CALL HookHandler();

		void UF_API_CALL addHook( const uf::Hook::name_t& name, const uf::Hook::function_t& function );
		const uf::HookHandler::map_t& UF_API_CALL getHooks() const;
		const uf::HookHandler::vector_t& UF_API_CALL getHooks( const uf::Hook::name_t& name ) const;
		bool UF_API_CALL hookExists( const uf::Hook::name_t& name ) const;

		void UF_API_CALL addAlias( const uf::Hook::name_t& name, const uf::Hook::name_t& target, const uf::Hook::argument_t& arg = uf::Hook::argument_t() );
		const uf::HookHandler::alias_map_t& UF_API_CALL getAliases() const;
		const uf::HookHandler::alias_vector_t& UF_API_CALL getAliases( const uf::Hook::name_t& name ) const;
		bool UF_API_CALL aliasExists( const uf::Hook::name_t& name ) const;

		uf::HookHandler::return_vector_t UF_API_CALL call( const uf::Hook::name_t& name, const uf::Hook::argument_t& argument = uf::Hook::argument_t() );
	};
*/
	extern UF_API uf::HookHandler hooks;
}