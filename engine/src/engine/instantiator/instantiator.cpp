#include <uf/engine/instantiator/instantiator.h>
#include <assert.h>

std::unordered_map<std::type_index, std::string>* uf::instantiator::names = NULL;
std::unordered_map<std::string, uf::instantiator::function_t>* uf::instantiator::map = NULL;

uf::Entity* uf::instantiator::alloc( size_t size ) {
/*
	uf::Entity* pointer = (uf::Entity*) uf::Entity::memoryPool.alloc( NULL, size );
	std::cout << "malloc uf::Entity: "<< pointer <<" (size: " << size << ")" << std::endl;
	return pointer;
*/
//	uf::MemoryPool& memoryPool = uf::MemoryPool::global.size() > 0 ? uf::MemoryPool::global : uf::Entity::memoryPool;
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
/*
	uf::Entity* pointer = ( memoryPool.size() <= 0 ) ? (uf::Entity*) ::malloc( size ) : (uf::Entity*) memoryPool.alloc( NULL, size );
	// std::cout << "malloc uf::Entity: "<< pointer <<" (size: " << size << ")" << std::endl;
	return pointer;
*/
/*
	std::cout << "malloc uf::Entity (size: " << size << ")" << std::endl;
	if ( memoryPool.size() <= 0 ) return (uf::Entity*) ::malloc( size );
	return (uf::Entity*) memoryPool.alloc( NULL, size );
*/
}
void uf::instantiator::free( uf::Entity* pointer ) {
//	uf::MemoryPool& memoryPool = uf::MemoryPool::global.size() > 0 ? uf::MemoryPool::global : uf::Entity::memoryPool;
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
/*
	// std::cout << "free uf::Entity: " << pointer << std::endl;
	if ( !uf::Entity::memoryPool.free( pointer ) )
		::free(pointer);
*/
}
uf::Entity* uf::instantiator::instantiate( const std::string& name ) {
	// std::cout << "instantiating " << name << std::endl;
	auto& map = *uf::instantiator::map;
	assert( map.count(name) > 0 );
	return map[name]();
}