#pragma once

#include <uf/utils/component/component.h>

#include <functional>
#include <vector>
#include <typeindex>

namespace uf {
	class UF_API Entity;
	class UF_API Object;
}

namespace pod {
	struct UF_API Behavior {
		typedef std::type_index type_t;
		typedef std::function<void(uf::Object&)> function_t;

		type_t type = std::type_index(typeid(pod::Behavior));
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

		container_t& getBehaviors() {
			return this->m_behaviors;
		}

		bool hasBehavior( const pod::Behavior& );
		template<typename T>
		bool hasBehavior() {
			for ( auto& behavior : this->m_behaviors ) {
				if ( behavior.type == getType<T>() ) return true;
			}
			return false;
		}

		void addBehavior( const pod::Behavior& );
		void removeBehavior( const pod::Behavior& );
		
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
			this->removeBehavior(pod::Behavior{
				.type = getType<T>()
			});
		/*
			auto it = this->m_behaviors.begin();
			while ( it != this->m_behaviors.end() ) {
				if ( (it++)->type == getType<T>() ) break;
			}
			if ( it != this->m_behaviors.end() )
				this->m_behaviors.erase(it);
		*/
		}
	};

}

#include "macros.inl"