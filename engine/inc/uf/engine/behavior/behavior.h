#pragma once

#include <uf/utils/component/component.h>

#include <functional>
#include <vector>
#include <typeindex>


namespace pod {
	struct UF_API Behavior {
		typedef std::type_index type_t;
		typedef std::function<void()> function_t;

		type_t type;
		function_t initialize;
		function_t tick;
		function_t render;
		function_t destroy;
	};
}

namespace uf {
	class UF_API Behaviors : public uf::Component {
	protected:
		typedef std::vector<pod::Behavior> container_t;
		container_t m_behaviors;
	public:
		void initialize();
		void tick();
		void render();
		void destroy();

		void addBehavior( const pod::Behavior& );
		
		template<typename T>
		static pod::Behavior::type_t getType() {
			return std::type_index(typeid(T));
		}
		template<typename T>
		void addBehavior() {
			this->addBehavior(pod::Behavior{
				.type = getType<T>(),
				.initialize = T::initialize,
				.tick = T::tick,
				.render = T::render,
				.destroy = T::destroy,
			});
		}
		template<typename T>
		void removeBehavior() {
			auto it = this->m_behaviors.begin();
			while ( it != this->m_behaviors.end() ) {
				if ( (it++)->type == getType<T>() ) break;
			}
			if ( it != this->m_behaviors.end() )
				this->m_behaviors.erase(it);
		}
	};

}

#define UF_BEHAVIOR_ENTITY_HEADER( OBJ )\
	class UF_API OBJ ## Behavior {\
	public:\
		void initialize();\
		void tick();\
		void render();\
		void destroy();\
	};
