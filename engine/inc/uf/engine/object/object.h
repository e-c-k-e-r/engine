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

		bool reload( bool = false );
		bool load( const std::string&, bool = false );
		bool load( const uf::Serializer& );
		std::size_t loadChild( const std::string&, bool = true );
		std::size_t loadChild( const uf::Serializer&, bool = true );

		void queueHook( const std::string&, const std::string& = "", double = 0 );
		std::vector<std::string> callHook( const std::string&, const std::string& = "" );
		std::size_t addHook( const std::string&, const uf::HookHandler::Readable::function_t& );
	
		Object();
	#if UF_BEHAVIOR_VIRTUAL
		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual void destroy();
	#endif
	};
}

#include "behavior.h"