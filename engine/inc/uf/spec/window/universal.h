#pragma once

#include <uf/config.h>

#include <uf/utils/math/vector.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/string/string.h>
#include <uf/utils/memory/string.h>
#include <queue>

namespace spec {
	namespace uni {
		class UF_API Window {
		public:
			typedef void* 									handle_t;
			typedef void* 									context_t;
			typedef uf::String 								title_t;
			typedef pod::Vector2i 							vector_t;

			struct Event {
				uf::stl::string name;
				pod::Hook::userdata_t payload;
			};
		protected:
			std::queue<Event> m_events;
		public:
			void pushEvent( const uf::Hooks::name_t& name, const pod::Hook::userdata_t& payload );
			template<typename T> void pushEvent( const uf::Hooks::name_t& name, const T& payload );
			static bool focused;
		};
	}
}

#include "universal.inl"