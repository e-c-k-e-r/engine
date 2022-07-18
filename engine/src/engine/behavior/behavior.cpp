#include <uf/engine/behavior/behavior.h>
#include <uf/engine/object/object.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/thread/thread.h>

#define UF_GRAPH_PRINT_TRACE 0
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
uf::Behaviors::Graph& uf::Behaviors::getGraph() {
//	if ( m_graph.initialize.size() != m_behaviors.size() ) generateGraph(); // should never actually happen, so no need to actually check
	return m_graph;
}
const uf::Behaviors::Graph& uf::Behaviors::getGraph() const { return m_graph; }
void uf::Behaviors::generateGraph() {
	m_graph.initialize.clear();
	m_graph.tick.serial.clear();
	m_graph.tick.parallel.clear();
	m_graph.render.clear();
	m_graph.destroy.clear();

	m_graph.initialize.reserve( m_behaviors.size() );
	m_graph.tick.serial.reserve( m_behaviors.size() );
	m_graph.tick.parallel.reserve( m_behaviors.size() );
	m_graph.render.reserve( m_behaviors.size() );
	m_graph.destroy.reserve( m_behaviors.size() );

	const bool forwardIteration = true;
	uf::Object* self = (uf::Object*) this;
	for ( auto& behavior : m_behaviors ) {
		m_graph.initialize.emplace_back(behavior.initialize);
	#if 1
		if ( behavior.traits.ticks ) {
			auto& f = behavior.tick;
			if ( behavior.traits.multithread ) m_graph.tick.parallel.emplace_back(f); //m_graph.tickMT.emplace_back([f, self](){f(*self);});
			else m_graph.tick.serial.emplace_back(f);
		}
	#else
		if ( behavior.traits.ticks ) m_graph.tick.emplace_back(behavior.tick);
	#endif
		if ( behavior.traits.renders ) m_graph.render.emplace_back(behavior.render);
		m_graph.destroy.emplace_back(behavior.destroy);
	}

	// reverse order ticking (LIFO order iteration)
	if ( !forwardIteration ) {
		std::reverse( m_graph.initialize.begin(), m_graph.initialize.end() );
		std::reverse( m_graph.tick.serial.begin(), m_graph.tick.serial.end() );
		std::reverse( m_graph.tick.parallel.begin(), m_graph.tick.parallel.end() );
		std::reverse( m_graph.render.begin(), m_graph.render.end() );
		std::reverse( m_graph.destroy.begin(), m_graph.destroy.end() );
	}

//	uf::scene::invalidateGraphs();
}

#define UF_BEHAVIOR_POLYFILL UF_BEHAVIOR_POLYFILL_GRAPH
#define UF_BEHAVIOR_POLYFILL_SAFE(f)\
	const bool forwardIteration = true;\
	if ( forwardIteration ) for ( auto& behavior : m_behaviors ) behavior.f(self);\
	else for ( auto it = m_behaviors.rbegin(); it != m_behaviors.rend(); ++it ) it->f(self);

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
//	if ( !m_graph.tickMT.empty() ) uf::thread::queue(m_graph.tickMT);
	if ( m_graph.tick.serial.empty() && m_graph.tick.parallel.empty() ) return;
#if UF_GRAPH_PRINT_TRACE
	UF_TIMER_MULTITRACE_START("Starting tick " << self.getName() << ": " << self.getUid());
//	for ( auto& behavior : m_behaviors ) if ( behavior.traits.ticks ) UF_MSG_DEBUG( behavior.type.name() );
	for ( auto& fun : m_graph.tick.serial ) {
		fun(self);
		UF_TIMER_MULTITRACE("");
	}
	for ( auto& fun : m_graph.tick.parallel ) {
		fun(self);
		UF_TIMER_MULTITRACE("");
	}
	UF_TIMER_MULTITRACE_END("Finished tick " << self.getName() << ": " << self.getUid())
#else
	UF_BEHAVIOR_POLYFILL(tick.serial)
	UF_BEHAVIOR_POLYFILL(tick.parallel)
#endif
}
void uf::Behaviors::render() {
	uf::Object& self = *((uf::Object*) this);
	if ( !self.isValid() ) return;
	if ( m_graph.render.empty() ) return;
#if UF_GRAPH_PRINT_TRACE
	UF_TIMER_MULTITRACE_START("Starting render " << self.getName() << ": " << self.getUid());
	for ( auto& fun : m_graph.render ) {
		fun(self);
		UF_TIMER_MULTITRACE("");
	}
	UF_TIMER_MULTITRACE_END("Finished render " << self.getName() << ": " << self.getUid())
#else
	UF_BEHAVIOR_POLYFILL(render)
#endif
}
void uf::Behaviors::destroy() {
	uf::Object& self = *((uf::Object*) this);
	if ( !self.isValid() ) return;
	UF_BEHAVIOR_POLYFILL(destroy)
	m_behaviors.clear();
	m_graph.initialize.clear();
	m_graph.tick.serial.clear();
	m_graph.tick.parallel.clear();
	m_graph.render.clear();
	m_graph.destroy.clear();
}

#if 0
void pod::Behavior::Metadata::serialize( uf::Object&, uf::Serializer& ) {}
void pod::Behavior::Metadata::serialize( uf::Object& self ) { return serialize( self, self.getComponent<uf::Serializer>() ); }
void pod::Behavior::Metadata::deserialize( uf::Object&, uf::Serializer& ) {}
void pod::Behavior::Metadata::deserialize( uf::Object& self ) { return deserialize( self, self.getComponent<uf::Serializer>() ); }
#endif