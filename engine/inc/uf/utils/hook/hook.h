#pragma once

#include <uf/config.h>

#include <uf/utils/userdata/userdata.h> 	// uf::Userdata

#include <string> 							// std::string
#include <functional> 						// std::function
#include <vector> 							// std::vector
#include <unordered_map> 					// std::unordered_map

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
		return_t call( const name_t& name, const ext::json::Value& );
		return_t call( const name_t& name, const uf::Serializer& );

		size_t addHook( const name_t& name, const std::function<void()>& callback );

		template<typename Arg>
		size_t addHook( const uf::Hooks::name_t& name, const std::function<void(Arg)>& callback );
		template<typename R, typename Arg>
		size_t addHook( const uf::Hooks::name_t& name, const std::function<R(Arg)>& callback );
		template<typename Function>
		size_t addHook( const uf::Hooks::name_t& name, const Function& lambda );

		template<typename T>
		return_t call( const name_t& name, const T& payload );
	};
	
	extern UF_API uf::Hooks hooks;
}
/*
namespace uf {
	namespace HookHandler {
		namespace Readable {
			typedef std::function<std::string(const std::string&)> function_t;
		}
	}
}
*/
#include "function_traits.inl"
#include "hook.inl"