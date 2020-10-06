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

#define UF_BEHAVIOR_POLYFILL(f)\
	uf::Object& self = *((uf::Object*) this);\
	bool headLoopChildren = true;\
	bool forwardIteration = true;\
	bool multithreading = false;\
	if ( this->hasComponent<uf::Serializer>() ) {\
		auto& metadata = this->getComponent<uf::Serializer>();\
		if ( !metadata["system"]["behavior"][#f]["head loop children"].isNull() )\
			headLoopChildren = metadata["system"]["behavior"][#f]["head loop children"].asBool();\
		if ( !metadata["system"]["behavior"][#f]["forward iteration"].isNull() )\
			forwardIteration = metadata["system"]["behavior"][#f]["forward iteration"].asBool();\
		if ( !metadata["system"]["behavior"][#f]["multithreading"].isNull() )\
			multithreading = metadata["system"]["behavior"][#f]["multithreading"].asBool();\
	}\
	auto function = [&]() -> int {\
		if ( headLoopChildren ) {\
			if ( forwardIteration )\
				for ( auto& behavior : this->m_behaviors )\
					behavior.f(self);\
			else\
				for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it )\
					it->f(self);\
		} else {\
			if ( forwardIteration ) {\
				for ( auto it = this->m_behaviors.begin() + 1; it != this->m_behaviors.end(); ++it )\
					it->f(self);\
				if ( this->m_behaviors.begin() != this->m_behaviors.end() ) {\
					this->m_behaviors.begin()->f(self);\
				}\
			} else {\
				for ( auto it = this->m_behaviors.rbegin() + 1; it != this->m_behaviors.rend(); ++it )\
					it->f(self);\
				if ( this->m_behaviors.rbegin() != this->m_behaviors.rend() ) {\
					this->m_behaviors.rbegin()->f(self);\
				}\
			}\
		}\
		return 0;\
	};\
	if ( multithreading ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();

#define UF_BEHAVIOR_POLYFILL_FAST(f)\
	uf::Object& self = *((uf::Object*) this);\
	bool headLoopChildren = true;\
	bool forwardIteration = true;\
	bool multithreading = false;\
	if ( this->hasComponent<uf::Serializer>() ) {\
		auto& metadata = this->getComponent<uf::Serializer>();\
		if ( !metadata["system"]["behavior"][#f]["head loop children"].isNull() )\
			headLoopChildren = metadata["system"]["behavior"][#f]["head loop children"].asBool();\
		if ( !metadata["system"]["behavior"][#f]["forward iteration"].isNull() )\
			forwardIteration = metadata["system"]["behavior"][#f]["forward iteration"].asBool();\
		if ( !metadata["system"]["behavior"][#f]["multithreading"].isNull() )\
			multithreading = metadata["system"]["behavior"][#f]["multithreading"].asBool();\
	}\
	if ( headLoopChildren ) {\
		if ( forwardIteration ) {\
			auto it = this->m_behaviors.begin();\
			if ( it != this->m_behaviors.end() ) {\
				this->m_behaviors.begin()->f(self);\
			}\
			for ( ++it; it != this->m_behaviors.end(); ++it ) {\
				auto function = [&]() -> int {\
					it->f(self);\
					return 0;\
				};\
				if ( multithreading ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();\
			}\
		} else {\
			auto it = this->m_behaviors.rbegin();\
			if ( it != this->m_behaviors.rend() ) {\
				this->m_behaviors.rbegin()->f(self);\
			}\
			for ( ++it; it != this->m_behaviors.rend(); ++it ) {\
				auto function = [&]() -> int {\
					it->f(self);\
					return 0;\
				};\
				if ( multithreading ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();\
			}\
		}\
	} else {\
		if ( forwardIteration ) {\
			for ( auto it = this->m_behaviors.begin() + 1; it != this->m_behaviors.end(); ++it ) {\
				auto function = [&]() -> int {\
					it->f(self);\
					return 0;\
				};\
				if ( multithreading ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();\
			}\
			if ( this->m_behaviors.begin() != this->m_behaviors.end() ) {\
				this->m_behaviors.begin()->f(self);\
			}\
		} else {\
			for ( auto it = this->m_behaviors.rbegin() + 1; it != this->m_behaviors.rend(); ++it ) {\
				auto function = [&]() -> int {\
					it->f(self);\
					return 0;\
				};\
				if ( multithreading ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();\
			}\
			if ( this->m_behaviors.rbegin() != this->m_behaviors.rend() ) {\
				this->m_behaviors.rbegin()->f(self);\
			}\
		}\
	}


void uf::Behaviors::initialize() {
	UF_BEHAVIOR_POLYFILL(initialize)
}
void uf::Behaviors::tick() {
	UF_BEHAVIOR_POLYFILL(tick)
}
void uf::Behaviors::render() {
	UF_BEHAVIOR_POLYFILL(render)
}
void uf::Behaviors::destroy() {
	UF_BEHAVIOR_POLYFILL(destroy)
}