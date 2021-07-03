#include <uf/engine/behavior/behavior.h>
#include <uf/engine/object/object.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/thread/thread.h>

#define UF_BEHAVIORS_REMOVE_STL_FIND 1

uf::Behaviors::container_t& uf::Behaviors::getBehaviors() { return m_behaviors; }
const uf::Behaviors::container_t& uf::Behaviors::getBehaviors() const { return m_behaviors; }
bool uf::Behaviors::hasBehavior( const pod::Behavior& target ) {
	auto& type = target.type;
	for ( auto& behavior : m_behaviors ) if ( behavior.type == type ) return true;
	return false;
}
void uf::Behaviors::addBehavior( const pod::Behavior& behavior ) {
	if ( hasBehavior( behavior ) ) return;
	m_behaviors.emplace_back(behavior);
	generateGraph();
}
void uf::Behaviors::removeBehavior( const pod::Behavior& behavior ) {
	auto& type = behavior.type;
#if UF_BEHAVIORS_REMOVE_STL_FIND
	auto it = std::find_if( m_behaviors.begin(), m_behaviors.end(), [&]( const pod::Behavior& b ) { return type == behavior.type; } );
#else
	auto it = m_behaviors.begin();
	while ( it != m_behaviors.end() ) if ( (it++)->type == behavior.type ) break;
#endif
	if ( it == m_behaviors.end() ) return;
	m_behaviors.erase(it);
	generateGraph();
}
void uf::Behaviors::generateGraph() {
	m_graph.initialize.clear();
	m_graph.tick.clear();
	m_graph.tickMT.clear();
	m_graph.render.clear();
	m_graph.destroy.clear();

	m_graph.initialize.reserve( m_behaviors.size() );
	m_graph.tick.reserve( m_behaviors.size() );
	m_graph.tickMT.reserve( m_behaviors.size() );
	m_graph.render.reserve( m_behaviors.size() );
	m_graph.destroy.reserve( m_behaviors.size() );

	const bool headLoopChildren = true;
	const bool forwardIteration = true;
	uf::Object* self = (uf::Object*) this;
	for ( auto& behavior : m_behaviors ) {
		m_graph.initialize.emplace_back(behavior.initialize);
		if ( behavior.traits.ticks ) {
			auto& f = behavior.tick;
			if ( behavior.traits.multithread ) m_graph.tickMT.emplace_back([f, self](){f(*self);});
			else m_graph.tick.emplace_back(f);
		}
		if ( behavior.traits.renders ) m_graph.render.emplace_back(behavior.render);
		m_graph.destroy.emplace_back(behavior.destroy);
	}

	// reverse order ticking (LIFO order iteration)
	if ( !forwardIteration ) {
		std::reverse( m_graph.initialize.begin(), m_graph.initialize.end() );
		std::reverse( m_graph.tick.begin(), m_graph.tick.end() );
		std::reverse( m_graph.tickMT.begin(), m_graph.tickMT.end() );
		std::reverse( m_graph.render.begin(), m_graph.render.end() );
		std::reverse( m_graph.destroy.begin(), m_graph.destroy.end() );
	}
	// EntityBehavior which ticks children is always the first behavior, so relocate accordingly
	if ( !headLoopChildren && forwardIteration ) {
		std::rotate( m_graph.initialize.begin(), m_graph.initialize.begin() + 1, m_graph.initialize.end() );
		std::rotate( m_graph.tick.begin(), m_graph.tick.begin() + 1, m_graph.tick.end() );
		std::rotate( m_graph.tickMT.begin(), m_graph.tickMT.begin() + 1, m_graph.tickMT.end() );
		std::rotate( m_graph.render.begin(), m_graph.render.begin() + 1, m_graph.render.end() );
		std::rotate( m_graph.destroy.begin(), m_graph.destroy.begin() + 1, m_graph.destroy.end() );
	}
}

#define UF_BEHAVIOR_POLYFILL UF_BEHAVIOR_POLYFILL_FAST
#define UF_BEHAVIOR_POLYFILL_SAFE(f)\
	const bool headLoopChildren = true;\
	const bool forwardIteration = true;\
	if ( headLoopChildren ) {\
		if ( forwardIteration ) for ( auto& behavior : m_behaviors ) behavior.f(self);\
		else for ( auto it = m_behaviors.rbegin(); it != m_behaviors.rend(); ++it ) it->f(self);\
	} else {\
		if ( forwardIteration ) {\
			auto it = m_behaviors.begin();\
			for ( ++it; it != m_behaviors.end(); ++it ) it->f(self);\
			if ( (it = m_behaviors.begin()) != m_behaviors.end() ) it->f(self);\
		} else {\
			auto it = m_behaviors.rbegin();\
			for ( ++it; it != m_behaviors.rend(); ++it ) it->f(self);\
			if ( (it = m_behaviors.rbegin()) != m_behaviors.rend() ) it->f(self);\
		}\
	}
#define UF_BEHAVIOR_POLYFILL_FAST(f) for ( auto& behavior : m_behaviors ) behavior.f(self);
#define UF_BEHAVIOR_POLYFILL_GRAPH(f) for ( auto& fun : m_graph.f ) fun(self);

void uf::Behaviors::initialize() {
	uf::Object& self = *((uf::Object*) this);
//	if ( !self.isValid() ) return;
	UF_BEHAVIOR_POLYFILL(initialize)
}
void uf::Behaviors::tick() {
	uf::Object& self = *((uf::Object*) this);
	if ( !self.isValid() ) return;
//	if ( !m_graph.tickMT.empty() ) uf::thread::batchWorkers(m_graph.tickMT, false);
	UF_BEHAVIOR_POLYFILL(tick)
}
void uf::Behaviors::render() {
	uf::Object& self = *((uf::Object*) this);
	if ( !self.isValid() ) return;
	UF_BEHAVIOR_POLYFILL(render)
}
void uf::Behaviors::destroy() {
	uf::Object& self = *((uf::Object*) this);
	if ( !self.isValid() ) return;
	UF_BEHAVIOR_POLYFILL(destroy)
}