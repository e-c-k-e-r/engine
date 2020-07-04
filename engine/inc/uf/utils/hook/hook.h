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
		std::size_t addHook( const Optimal::name_t& name, const Optimal::function_t& callback ); 	// Adds a hook that receives optimal data
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
		std::vector<Readable::return_t> call( const Readable::name_t& name, const Readable::argument_t& argument );
		void call( const Optimal::name_t& name, const Optimal::argument_t& argument );
	};
	class UF_API Hooks {
	public:
		typedef std::unordered_map<std::string, std::vector<std::size_t>> container_t;
	protected:
		uf::Hooks::container_t m_container;
	public:
		~Hooks();
		void clear();
		std::size_t addHook( const uf::HookHandler::Readable::name_t& name, const uf::HookHandler::Readable::function_t& callback ); 	// Adds a hook that receives readable data
		std::size_t addHook( const uf::HookHandler::Optimal::name_t& name, const uf::HookHandler::Optimal::function_t& callback ); 	// Adds a hook that receives optimal data
	};

	extern UF_API uf::HookHandler hooks;
}