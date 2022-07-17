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
		static bool assertionLoad;

		Object();

		bool reload( bool = false );
		bool load( const uf::stl::string&, bool = false );
		bool load( const uf::Serializer& );

		uf::Object& loadChild( const uf::Serializer&, bool = true );
		uf::Object* loadChildPointer( const uf::Serializer&, bool = true );
		std::size_t loadChildUid( const uf::Serializer&, bool = true );

		uf::Object& loadChild( const uf::stl::string&, bool = true );
		uf::Object* loadChildPointer( const uf::stl::string&, bool = true );
		std::size_t loadChildUid( const uf::stl::string&, bool = true );
		
		template<typename T>
		T loadChild( const uf::Serializer&, bool = true );
		template<typename T>
		T loadChild( const uf::stl::string&, bool = true );

		uf::stl::string formatHookName( const uf::stl::string& name );
		static uf::stl::string formatHookName( const uf::stl::string& name, size_t uid, bool fetch = true );

		template<typename T> size_t addHook( const uf::stl::string& name, T function );

		uf::Hooks::return_t callHook( const uf::stl::string& );
		uf::Hooks::return_t callHook( const uf::stl::string&, const pod::Hook::userdata_t& );
		template<typename T> uf::Hooks::return_t callHook( const uf::stl::string& name, const T& payload );

		void queueHook( const uf::stl::string&, float = 0 );
		void queueHook( const uf::stl::string&, double );
		void queueHook( const uf::stl::string&, const ext::json::Value& json, float = 0 );
		void queueHook( const uf::stl::string&, const ext::json::Value& json, double );
		
		template<typename T>
		void queueHook( const uf::stl::string&, const T&, float = 0 );

		uf::stl::string resolveURI( const uf::stl::string& filename, const uf::stl::string& root = "" );

		void queueDeletion();
	};
}

namespace uf {
	namespace string {
		uf::stl::string UF_API toString( const uf::Object& object );
	}
}

#include "object.inl"
#include "behavior.h"
#include "payloads.h"