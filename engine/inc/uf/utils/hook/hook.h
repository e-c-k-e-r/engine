#pragma once

#include <uf/config.h>

#include <uf/utils/userdata/userdata.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/hash.h>

#include <functional>
#include <type_traits>

#include "payloads.h"

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
			UF_USERDATA_CTTI_TYPE hash;
			size_t size;
		} type;
		function_t callback;
	};
}

namespace uf {
	class UF_API Hooks {
	public:
		//typedef uf::hashed_string name_t;
		typedef uf::stl::string name_t;
		typedef uf::stl::vector<pod::Hook> hooks_t;
		typedef uf::stl::unordered_map<name_t, hooks_t> container_t;

		typedef pod::Hook::userdata_t argument_t;
		typedef uf::stl::vector<pod::Hook::userdata_t> return_t;
	protected:
		static size_t uids;
		uf::Hooks::container_t m_container;
	public:
		bool exists( const name_t& name );
		void removeHook( const name_t& name, size_t uid );
		void removeHooks();

		size_t addHook( const name_t& name, const pod::Hook::function_t& callback, const pod::Hook::Type& = {UF_USERDATA_CTTI(void), 0} );
		size_t addHook( const name_t& name, const std::function<void()>& callback );
		return_t call( const name_t& name, const argument_t& payload = {} );
		
		template<typename Arg> size_t addHook( const name_t& name, const std::function<void(Arg)>& callback );
		template<typename R, typename Arg> size_t addHook( const name_t& name, const std::function<R(Arg)>& callback );
		template<typename Function> size_t addHook( const name_t& name, const Function& lambda );
		template<typename T> return_t call( const name_t& name, const T& payload );
	};
	
	extern UF_API uf::Hooks hooks;
}

#include "function_traits.inl"
#include "hook.inl"