#pragma once

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

		void queueHook( const std::string&, const std::string& = "", double = 0 );
		std::vector<std::string> callHook( const std::string&, const std::string& = "" );
		std::size_t addHook( const std::string&, const uf::HookHandler::Readable::function_t& );
	};
}

#include "object.inl"
#include "behavior.h"