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

#define UF_HOOK_POINTERED_USERDATA 1

namespace pod {
	struct UF_API Hook {
	#if UF_HOOK_POINTERED_USERDATA
		typedef uf::PointeredUserdata userdata_t;
	#else
		typedef uf::Userdata userdata_t;
	#endif
		typedef std::function<pod::Hook::userdata_t(const pod::Hook::userdata_t&)> function_t;

		size_t uid;
		struct Type {
			size_t hash;
			size_t size;
		} type;
		function_t callback;
	};
}

namespace uf {
	class UF_API Hooks {
	public:
		typedef std::vector<pod::Hook> hooks_t;
		typedef std::unordered_map<std::string, hooks_t> container_t;

		typedef std::string name_t;
		typedef pod::Hook::userdata_t argument_t;
		typedef std::vector<pod::Hook::userdata_t> return_t;
	protected:
		static size_t uids;
		uf::Hooks::container_t m_container;
	public:
		bool exists( const name_t& name );
		size_t addHook( const name_t& name, const pod::Hook::function_t& callback, const pod::Hook::Type& = {0, 0} );
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