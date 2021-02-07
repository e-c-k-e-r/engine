#include <uf/engine/instantiator/instantiator.h>
#include <uf/engine/object/object.h>
#include <uf/engine/scene/scene.h>
#include <assert.h>

pod::NamedTypes<pod::Instantiator>* uf::instantiator::objects = NULL;
pod::NamedTypes<pod::Behavior>* uf::instantiator::behaviors = NULL;

uf::Entity* uf::instantiator::reuse( size_t size ) {
	uf::Entity* laxed = NULL;
	auto& allocations = uf::Entity::memoryPool.allocations();

	for ( auto& allocation : allocations ) {
		uf::Entity* e = (uf::Entity*) allocation.pointer;
		// no scenes
		if ( std::find( uf::scene::scenes.begin(), uf::scene::scenes.end(), (uf::Scene*) e ) != uf::scene::scenes.end() ) continue;
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
	for ( auto& allocation : allocations ) {
		uf::Entity* e = (uf::Entity*) allocation.pointer;
		// no scenes
		// if ( std::find( uf::scene::scenes.begin(), uf::scene::scenes.end(), (uf::Scene*) e ) != uf::scene::scenes.end() ) continue;
		// not current scene
		if ( e->getUid() == scene.getUid() ) continue;
		// only orphaned
		if ( e->hasParent() ) continue;
		// uninitialized
		if ( e->getName() == "Entity" && !e->isValid() ) continue;
		// uf::iostream << "Found orphan: " << e->getName() << ": " << e->getUid() << "\n";
		uf::instantiator::free( e );
		++collected;
	}
	return collected;
}

uf::Entity* uf::instantiator::alloc( size_t size ) {
	// auto* reused = reuse( size ); if ( reused ) return reused;
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = uf::Entity::memoryPool.size() > 0 ? uf::Entity::memoryPool : uf::MemoryPool::global;
	return (uf::Entity*) memoryPool.alloc( NULL, size );
#else
	uf::Entity* pointer = NULL;
	uf::MemoryPool* memoryPool = NULL;
	if ( uf::Entity::memoryPool.size() > 0 )
		memoryPool = &uf::Entity::memoryPool;
	else if ( uf::MemoryPool::global.size() > 0 )
		memoryPool = &uf::MemoryPool::global;
	if ( memoryPool ) pointer = (uf::Entity*) memoryPool->alloc( NULL, size );
	else pointer = (uf::Entity*) malloc( size );
	return pointer;
#endif
}
void uf::instantiator::free( uf::Entity* pointer ) {
	if ( !pointer ) return;
	if ( pointer->getUid() > 0 )
		pointer->destroy();

#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool& memoryPool = uf::Entity::memoryPool.size() > 0 ? uf::Entity::memoryPool : uf::MemoryPool::global;
	memoryPool.free( pointer );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( uf::Entity::memoryPool.size() > 0 )
		memoryPool = &uf::Entity::memoryPool;
	else if ( uf::MemoryPool::global.size() > 0 )
		memoryPool = &uf::MemoryPool::global;
	if ( memoryPool ) memoryPool->free( pointer );
	else ::free( pointer );
#endif
}
bool uf::instantiator::valid( uf::Entity* pointer ) {
#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool& memoryPool = uf::Entity::memoryPool.size() > 0 ? uf::Entity::memoryPool : uf::MemoryPool::global;
	return memoryPool.exists( pointer );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( uf::Entity::memoryPool.size() > 0 )
		memoryPool = &uf::Entity::memoryPool;
	else if ( uf::MemoryPool::global.size() > 0 )
		memoryPool = &uf::MemoryPool::global;
	return memoryPool ? memoryPool->exists( pointer ) : pointer && pointer->isValid();
#endif
}

void uf::instantiator::registerBinding( const std::string& object, const std::string& behavior ) {
	if ( !objects ) objects = new pod::NamedTypes<pod::Instantiator>;
/*
	if ( !uf::instantiator::objects->has( object ) ) {
	//	uf::instantiator::registerObject<uf::Object>( object );
	}
*/
	auto& instantiator = uf::instantiator::objects->get( object );
	instantiator.behaviors.emplace_back( behavior );
	
	if ( UF_INSTANTIATOR_ANNOUNCE ) std::cout << "Registered binding: " << object << " and " << behavior << ": " << instantiator.behaviors.size() << std::endl;
}

uf::Entity& uf::instantiator::instantiate( const std::string& name ) {
	if ( !uf::instantiator::objects->has( name ) ) {
		auto& object = uf::instantiator::instantiate<uf::Object>();
		return *((uf::Entity*) &object);
	}
	auto& instantiator = uf::instantiator::objects->get( name );
	auto& entity = *instantiator.function();
	bind( name, entity );
	return entity;
}

void uf::instantiator::bind( const std::string& name, uf::Entity& entity ) {
	// was actually a behavior name, single bind
	if ( !uf::instantiator::objects->has( name, false ) ) {
		if ( !uf::instantiator::behaviors->has( name, false ) ) return;
		auto& behavior = uf::instantiator::behaviors->get( name );
		entity.addBehavior(behavior);
		return;
	}
	auto& instantiator = uf::instantiator::objects->get( name );
	for ( auto& name : instantiator.behaviors ) {
		auto& behavior = uf::instantiator::behaviors->get( name );
		entity.addBehavior(behavior);
	}
}

void uf::instantiator::unbind( const std::string& name, uf::Entity& entity ) {
	// was actually a behavior name, single bind
	if ( !uf::instantiator::objects->has( name, false ) ) {
		if ( !uf::instantiator::behaviors->has( name, false ) ) return;
		auto& behavior = uf::instantiator::behaviors->get( name );
		entity.removeBehavior(behavior);
		return;
	}
	auto& instantiator = uf::instantiator::objects->get( name );
	for ( auto& name : instantiator.behaviors ) {
		auto& behavior = uf::instantiator::behaviors->get( name );
		entity.removeBehavior(behavior);
	}
}