#include <uf/engine/behavior/behavior.h>
#include <uf/engine/object/object.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/thread/thread.h>

bool uf::Behaviors::hasBehavior( const pod::Behavior& target ) {
	for ( auto& behavior : this->m_behaviors ) {
		if ( behavior.type == target.type ) return true;
	}
	return false;
}
void uf::Behaviors::addBehavior( const pod::Behavior& behavior ) {
	if ( hasBehavior( behavior ) ) return;
	this->m_behaviors.emplace_back(behavior);
}
void uf::Behaviors::removeBehavior( const pod::Behavior& behavior ) {
	//if ( hasBehavior( behavior ) ) return;
	// this->m_behaviors.emplace_back(behavior);
	auto it = this->m_behaviors.begin();
	while ( it != this->m_behaviors.end() ) {
		if ( it->type == behavior.type ) break;
		++it;
	}
	if ( it != this->m_behaviors.end() ) this->m_behaviors.erase(it);
}

#define FUNCTION_AS_VARIABLE(x) x

#define UF_BEHAVIOR_POLYFILL UF_BEHAVIOR_POLYFILL_FAST
/*
//	if ( this->hasComponent<uf::Serializer>() ) {\
//		auto& metadata = this->getComponent<uf::Serializer>();\
//		if ( !ext::json::isNull( metadata["system"]["behavior"][#f]["head loop children"] ) )\
//			headLoopChildren = metadata["system"]["behavior"][#f]["head loop children"].as<bool>();\
//		if ( !ext::json::isNull( metadata["system"]["behavior"][#f]["forward iteration"] ) )\
//			forwardIteration = metadata["system"]["behavior"][#f]["forward iteration"].as<bool>();\
//		if ( !ext::json::isNull( metadata["system"]["behavior"][#f]["multithreading"] ) )\
//			multithreading = metadata["system"]["behavior"][#f]["multithreading"].as<bool>();\
//	}\
*/

#define UF_BEHAVIOR_POLYFILL_SAFE(f)\
	bool headLoopChildren = true;\
	bool forwardIteration = true;\
	if ( headLoopChildren ) {\
		if ( forwardIteration )\
			for ( auto& behavior : this->m_behaviors ){\
				behavior.f(self);\
			}\
		else\
			for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it )\
				it->f(self);\
	} else {\
		if ( forwardIteration ) {\
			auto it = this->m_behaviors.begin();\
			for ( ++it; it != this->m_behaviors.end(); ++it )\
				it->f(self);\
			if ( (it = this->m_behaviors.begin()) != this->m_behaviors.end() ) {\
				it->f(self);\
			}\
		} else {\
			auto it = this->m_behaviors.rbegin();\
			for ( ++it; it != this->m_behaviors.rend(); ++it )\
				it->f(self);\
			if ( (it = this->m_behaviors.rbegin()) != this->m_behaviors.rend() ) {\
				it->f(self);\
			}\
		}\
	}

#define UF_BEHAVIOR_POLYFILL_FAST(f)\
	for ( auto& behavior : this->m_behaviors ) behavior.f(self);
/*
#define UF_BEHAVIOR_POLYFILL_FAST(f)\
	for ( auto& behavior : this->m_behaviors ) {\
		UF_TIMER_TRACE_INIT();\
		behavior.f(self);\
		UF_TIMER_TRACE(self.getName() << ": " << self.getUid() << " | " << behavior.type.name() );\
	}
*/

void uf::Behaviors::initialize() {
	uf::Object& self = *((uf::Object*) this);
//	if ( !self.isValid() ) return;
	UF_BEHAVIOR_POLYFILL(initialize)
}
void uf::Behaviors::tick() {
	uf::Object& self = *((uf::Object*) this);
	if ( !self.isValid() ) return;
//	UF_TIMER_TRACE_INIT();
//	UF_BEHAVIOR_POLYFILL(tick)
//	UF_TIMER_TRACE(self.getName() << ": " << self.getUid());

//	UF_TIMER_MULTITRACE_START("==== START TICKING BEHAVIORS ====");
	for ( auto& behavior : this->m_behaviors ) {
		behavior.tick(self);
//		UF_TIMER_MULTITRACE(behavior.type);
	}
//	UF_TIMER_MULTITRACE_END("==== END TICKING BEHAVIORS ====");
}
void uf::Behaviors::render() {
	uf::Object& self = *((uf::Object*) this);
	if ( !self.isValid() ) return;
//	UF_TIMER_TRACE_INIT();
	UF_BEHAVIOR_POLYFILL(render)
//	UF_TIMER_TRACE(self.getName() << ": " << self.getUid());
}
void uf::Behaviors::destroy() {
	uf::Object& self = *((uf::Object*) this);
	if ( !self.isValid() ) return;
	UF_BEHAVIOR_POLYFILL(destroy)
}