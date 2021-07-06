#pragma once

#include <uf/utils/component/component.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/memory/vector.h>

#include <functional>
#include <typeindex>

namespace uf {
	class UF_API Entity;
	class UF_API Object;
}

namespace pod {
	struct UF_API Behavior {
		typedef TYPE_INDEX_T type_t;
	//	typedef uf::stl::string type_t;
		struct Traits {
			bool ticks = true;
			bool renders = true;
			bool multithread = false;
		};
		struct Metadata {
		public:
		#if 0
			virtual void serialize( uf::Object&, uf::Serializer& );
			virtual void deserialize( uf::Object&, uf::Serializer& );

			inline void serialize( uf::Object& self );
			inline void deserialize( uf::Object& self );
		#endif
		};
	//	struct Header {
		//	type_t type = ""; 
			type_t type = TYPE(void);
			Traits traits{};
	//	} header;
		typedef std::function<void(uf::Object&)> function_t;
	//	struct Functions {
			function_t initialize = function_t();
			function_t tick = function_t();
			function_t render = function_t();
			function_t destroy = function_t();
	//	} functions;
	};
}

namespace uf {
	class UF_API Behaviors : public uf::Component {
	protected:
		typedef uf::stl::vector<pod::Behavior> container_t;
	//	typedef uf::stl::vector<pod::Behavior::Header> container_t;
		container_t m_behaviors;
		struct {
			typedef pod::Behavior::function_t value_t;
		//	typedef uint_fast8_t value_t;
			uf::stl::vector<value_t> initialize;
			uf::stl::vector<value_t> tick;
			uf::stl::vector<pod::Thread::function_t> tickMT;
			uf::stl::vector<value_t> render;
			uf::stl::vector<value_t> destroy;
		} m_graph;
	public:
		void initialize();
		void tick();
		void render();
		void destroy();

		container_t& getBehaviors();
		const container_t& getBehaviors() const;

		bool hasBehavior( const pod::Behavior& );
		void addBehavior( const pod::Behavior& );
		void removeBehavior( const pod::Behavior& );
		
		void generateGraph();
	#if 0
		template<typename T>
		bool hasBehavior() {
			for ( auto& behavior : this->m_behaviors ) {
				if ( behavior.type == getType<T>() ) return true;
			}
			return false;
		}
		template<typename T>
		static pod::Behavior::type_t getType() {
			return T::type; // TYPE(T);
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
		}
	#endif
	};

}

#include "macros.inl"