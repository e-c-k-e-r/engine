#pragma once

#include "behavior.h"
#include <uf/engine/entity/entity.h>
#include <uf/engine/instantiator/instantiator.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>

#include <typeindex>
#include <functional>

namespace uf {
	class UF_API Object : public uf::Entity {
	public:
		static uf::Timer<long long> timer;
		Object();

		bool reload( bool = false );
		bool load( const std::string&, bool = false );
		bool load( const uf::Serializer& );

		uf::Object& loadChild( const uf::Serializer&, bool = true );
		uf::Object* loadChildPointer( const uf::Serializer&, bool = true );
		std::size_t loadChildUid( const uf::Serializer&, bool = true );

		uf::Object& loadChild( const std::string&, bool = true );
		uf::Object* loadChildPointer( const std::string&, bool = true );
		std::size_t loadChildUid( const std::string&, bool = true );
		
		template<typename T>
		T loadChild( const uf::Serializer&, bool = true );
		template<typename T>
		T loadChild( const std::string&, bool = true );

		std::string formatHookName( const std::string& name );
		static std::string formatHookName( const std::string& name, size_t uid, bool fetch = true );

		void queueHook( const std::string&, const ext::json::Value& = ext::json::null(), double = 0 );
		uf::Hooks::return_t callHook( const std::string& );
		uf::Hooks::return_t callHook( const std::string&, const ext::json::Value& );
		uf::Hooks::return_t callHook( const std::string&, const uf::Serializer& );
		
		template<typename T> size_t addHook( const std::string& name, T function );
		template<typename T> uf::Hooks::return_t callHook( const std::string& name, const T& payload );

		std::string grabURI( const std::string& filename, const std::string& root = "" );
	};
}

namespace uf {
	namespace string {
		std::string UF_API toString( const uf::Object& object );
	}
}

#include "object.inl"
#include "behavior.h"