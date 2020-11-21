#pragma once

#include <uf/config.h>

#include <uf/utils/userdata/userdata.h> 	// uf::Userdata

#include <string> 							// std::string
#include <functional> 						// std::function
#include <vector> 							// std::vector
#include <unordered_map> 					// std::unordered_map

#if 0
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

		std::vector<Readable::return_t> call( const Readable::name_t& name );
		std::vector<Readable::return_t> call( const Readable::name_t& name, const Readable::argument_t& argument );
		std::vector<Optimal::return_t> call( const Optimal::name_t& name, const Optimal::argument_t& argument );
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
#endif


#include <uf/utils/userdata/userdata.h>
#include <uf/utils/serialize/serializer.h>
#include <type_traits>

namespace pod {
	struct UF_API Hook {
		typedef std::function<uf::Userdata(const uf::Userdata&)> function_t;

		size_t uid;
		size_t type;
		function_t callback;
	};
}

namespace uf {
	class UF_API Hooks {
	public:
		typedef std::vector<pod::Hook> hooks_t;
		typedef std::unordered_map<std::string, hooks_t> container_t;

		typedef std::string name_t;
		typedef uf::Userdata argument_t;
		typedef std::vector<uf::Userdata> return_t;
	protected:
		static size_t uids;
		uf::Hooks::container_t m_container;
	public:
		bool exists( const name_t& name );
		size_t addHook( const name_t& name, const pod::Hook::function_t& callback, size_t type = 0 );
		void removeHook( const name_t& name, size_t uid );
		return_t call( const name_t& name, const argument_t& payload );
		
		return_t call( const name_t& name, const std::string& = "" );
		return_t call( const name_t& name, const uf::Serializer& );

		size_t addHook( const name_t& name, const std::function<void()>& callback );
	/*
		size_t addHook( const name_t& name, const std::function<void(const std::string&)>& callback );
		size_t addHook( const name_t& name, const std::function<std::string(const std::string&)>& callback );
	*/
		template<typename Arg>
		size_t addHook( const uf::Hooks::name_t& name, const std::function<void(Arg)>& callback );
		template<typename R, typename Arg>
		size_t addHook( const uf::Hooks::name_t& name, const std::function<R(Arg)>& callback );
	/*
		template<typename R, typename ...Args>
		size_t addHook( const uf::Hooks::name_t& name, const std::function<R(Args...)>& callback );
	*/
		template<typename Function>
		size_t addHook( const uf::Hooks::name_t& name, const Function& lambda );

		template<typename T>
		return_t call( const name_t& name, const T& payload );
	};
	
	extern UF_API uf::Hooks hooks;
}

namespace uf {
	namespace HookHandler {
		namespace Readable {
			typedef std::function<std::string(const std::string&)> function_t;
		}
	}
}

#include "function_traits.inl"
#include "hook.inl"