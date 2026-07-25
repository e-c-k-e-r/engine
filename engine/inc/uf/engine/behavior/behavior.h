#pragma once

#include <uf/utils/component/component.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/memory/vector.h>

#include <functional>
#include <typeindex>

namespace uf {
	class UF_API Entity;
	typedef uf::Entity Object;
}

namespace pod {
	struct UF_API Behavior {
		typedef TYPE_INDEX_T type_t;

		struct Traits {
			bool ticks = true;
			bool renders = true;
			uf::stl::string thread = "";
		};
		
		struct Metadata {
			// intentionally left blank
		};

		type_t type = TYPE(void);
		Traits traits{};

	//	typedef std::function<void(uf::Object&)> function_t;
		typedef void(*function_t)(uf::Object&);
		function_t initialize = NULL;
		function_t tick = NULL;
		function_t render = NULL;
		function_t destroy = NULL;
	};
}

namespace uf {
	class UF_API Behaviors : public uf::Component {
	protected:
		typedef uf::stl::vector<pod::Behavior> container_t;

		container_t m_behaviors;
		struct Graph {
			typedef pod::Behavior::function_t value_t;

			uf::stl::vector<value_t> initialize;
			uf::stl::vector<value_t> render;
			uf::stl::vector<value_t> destroy;
			struct {
				uf::stl::vector<value_t> serial;
				uf::stl::vector<value_t> parallel;
			} tick;
		} m_graph;
	public:
		void initialize();
		void tick();
		void render();
		void destroy();

		container_t& getBehaviors();
		const container_t& getBehaviors() const;

		Graph& getGraph();
		const Graph& getGraph() const;

		bool hasBehavior( const pod::Behavior& );
		void addBehavior( const pod::Behavior& );
		void removeBehavior( const pod::Behavior& );
		
		void generateGraph();
	};
}

#include "macros.inl"