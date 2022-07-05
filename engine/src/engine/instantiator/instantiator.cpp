#include <uf/engine/instantiator/instantiator.h>
#include <uf/engine/object/object.h>
#include <uf/engine/object/behavior.h>
#include <uf/engine/scene/scene.h>
#include <assert.h>

pod::NamedTypes<pod::Instantiator>* uf::instantiator::objects = NULL;
//pod::NamedTypes<pod::Behavior>* uf::instantiator::behaviors = NULL;
uf::stl::unordered_map<uf::stl::string, pod::Behavior>* uf::instantiator::behaviors = NULL;

uf::Entity* uf::instantiator::reuse( size_t size ) {
	uf::Entity* laxed = NULL;
	auto& allocations = uf::Entity::memoryPool.allocations();
	for ( auto& allocation : allocations ) {
		uf::Entity* e = (uf::Entity*) (allocation.pointer);
		// no scenes
		if ( std::find( uf::scene::scenes.begin(), uf::scene::scenes.end(), (uf::Scene*) e ) != uf::scene::scenes.end() ) continue;
		if ( e->hasComponent<uf::ObjectBehavior::Metadata>() ) {
			auto& metadata = e->getComponent<uf::ObjectBehavior::Metadata>();
			if ( metadata.system.markedForDeletion ) {
				if ( allocation.size == size ) return e;
				if ( allocation.size > size ) laxed = e;
				continue;
			}
		}
		// only orphaned
		if ( e->hasParent() ) continue;
		// only destroyed entities 
		if ( e->getName() == "Entity" || e->getUid() > 0 ) continue;

		if ( allocation.size == size ) return e;
		if ( allocation.size > size ) laxed = e;
	}
	return laxed;
}
size_t uf::instantiator::collect( uint8_t level ) {
	size_t collected = 0;
	auto& allocations = uf::Entity::memoryPool.allocations();
	auto& scene = uf::scene::getCurrentScene();

	uf::stl::vector<uintptr_t> queued;
	for ( auto& allocation : allocations ) {
		uf::Entity* e = (uf::Entity*) (allocation.pointer);
		// no scenes
		// if ( std::find( uf::scene::scenes.begin(), uf::scene::scenes.end(), (uf::Scene*) e ) != uf::scene::scenes.end() ) continue;
		// not current scene
		if ( e->getUid() == scene.getUid() ) continue;
		if ( e->hasComponent<uf::ObjectBehavior::Metadata>() ) {
			auto& metadata = e->getComponent<uf::ObjectBehavior::Metadata>();
			if ( metadata.system.markedForDeletion ) goto FREE;
		}
		// only orphaned
		if ( e->hasParent() ) continue;
		// uninitialized
		if ( e->getName() == "Entity" && !e->isValid() ) continue;
		// uf::iostream << "Found orphan: " << e->getName() << ": " << e->getUid() << "\n";
	FREE:
		queued.emplace_back( allocation.pointer );
	}
	for ( auto& p : queued ) uf::instantiator::free( (uf::Entity*) (p) );
	return queued.size();
}

uf::Entity* uf::instantiator::alloc( size_t size ) {
	// auto* reused = reuse( size ); if ( reused ) return reused;
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = uf::Entity::memoryPool.size() > 0 ? uf::Entity::memoryPool : uf::memoryPool::global;
	return (uf::Entity*) memoryPool.alloc( size );
#else
	uf::Entity* pointer = NULL;
	uf::MemoryPool* memoryPool = NULL;
	if ( uf::Entity::memoryPool.size() > 0 ) memoryPool = &uf::Entity::memoryPool;
	else if ( uf::memoryPool::global.size() > 0 ) memoryPool = &uf::memoryPool::global;

	if ( memoryPool ) pointer = (uf::Entity*) memoryPool->alloc( size );
	else pointer = (uf::Entity*) uf::allocator::malloc_m( size );

	return pointer;
#endif
}
void uf::instantiator::free( uf::Entity* pointer ) {
	if ( !pointer ) return;
	if ( pointer->isValid() ) pointer->destroy();
	pointer->~Entity();

#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool& memoryPool = uf::Entity::memoryPool.size() > 0 ? uf::Entity::memoryPool : uf::memoryPool::global;
	memoryPool.free( (void*) pointer );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( uf::Entity::memoryPool.size() > 0 ) memoryPool = &uf::Entity::memoryPool;
	else if ( uf::memoryPool::global.size() > 0 ) memoryPool = &uf::memoryPool::global;

	if ( memoryPool ) memoryPool->free( (void*) pointer );
	else uf::allocator::free_m( (void*) pointer );

#endif
}
bool uf::instantiator::valid( uf::Entity* pointer ) {
#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool& memoryPool = uf::Entity::memoryPool.size() > 0 ? uf::Entity::memoryPool : uf::memoryPool::global;
	return memoryPool.exists( pointer );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( uf::Entity::memoryPool.size() > 0 ) memoryPool = &uf::Entity::memoryPool;
	else if ( uf::memoryPool::global.size() > 0 ) memoryPool = &uf::memoryPool::global;

	return memoryPool ? memoryPool->exists( pointer ) : pointer && pointer->isValid();
#endif
}

void uf::instantiator::registerBehavior( const uf::stl::string& name, const pod::Behavior& behavior ) {
	if ( !uf::instantiator::behaviors ) uf::instantiator::behaviors = new uf::stl::unordered_map<uf::stl::string, pod::Behavior>;\
	(*uf::instantiator::behaviors)[name] = behavior;
#if UF_INSTANTIATOR_ANNOUNCE
	UF_MSG_DEBUG("Registered behavior for {} | {}", name, behavior.type);
#endif
}
void uf::instantiator::registerBinding( const uf::stl::string& object, const uf::stl::string& behavior ) {
	if ( !objects ) objects = new pod::NamedTypes<pod::Instantiator>;
	if ( !behaviors ) behaviors = new uf::stl::unordered_map<uf::stl::string, pod::Behavior>;
	auto& instantiator = uf::instantiator::objects->get( object );
	instantiator.behaviors.emplace_back( behavior );
	
#if UF_INSTANTIATOR_ANNOUNCE
	UF_MSG_DEBUG("Registered binding: {} and {}: {}", object, behavior, instantiator.behaviors.size());
#endif
}

uf::Entity& uf::instantiator::instantiate( const uf::stl::string& name ) {
	if ( !uf::instantiator::objects->has( name ) ) {
		auto& object = uf::instantiator::instantiate<uf::Object>();
		return *((uf::Entity*) &object);
	}
	auto& instantiator = uf::instantiator::objects->get( name );
	auto& entity = *instantiator.function();
	bind( name, entity );
	return entity;
}

void uf::instantiator::bind( const uf::stl::string& name, uf::Entity& entity ) {
	// was actually a behavior name, single bind
	if ( !uf::instantiator::objects->has( name, false ) ) {
		if ( uf::instantiator::behaviors->count( name ) == 0 ) return;
		auto& behavior = (*uf::instantiator::behaviors)[name];
	#if UF_INSTANTIATOR_ANNOUNCE
		UF_MSG_DEBUG("Attaching {} | {} to {}", name, behavior.type, entity.getName());
	#endif
		entity.addBehavior(behavior);
		return;
	}
	auto& instantiator = uf::instantiator::objects->get( name );
	for ( auto& name : instantiator.behaviors ) {
		auto& behavior = (*uf::instantiator::behaviors)[name];
	#if UF_INSTANTIATOR_ANNOUNCE
		UF_MSG_DEBUG("Attaching {} | {} to {}", name, behavior.type, entity.getName());
	#endif
		entity.addBehavior(behavior);
	}
}

void uf::instantiator::unbind( const uf::stl::string& name, uf::Entity& entity ) {
	// was actually a behavior name, single bind
	if ( !uf::instantiator::objects->has( name, false ) ) {
		if ( uf::instantiator::behaviors->count( name ) > 0 ) return;
		auto& behavior = (*uf::instantiator::behaviors)[name];
		entity.removeBehavior(behavior);
		return;
	}
	auto& instantiator = uf::instantiator::objects->get( name );
	for ( auto& name : instantiator.behaviors ) {
		auto& behavior = (*uf::instantiator::behaviors)[name];
		entity.removeBehavior(behavior);
	}
}