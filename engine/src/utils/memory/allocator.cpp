#include <uf/utils/memory/allocator.h>
#include <uf/utils/memory/pool.h>


bool uf::allocator::useMemoryPool = false;
void* uf::allocator::allocate( size_t n ) {
	return uf::allocator::useMemoryPool && uf::memoryPool::global.size() > 0 ? uf::memoryPool::global.alloc( nullptr, n ) : ::operator new( n );
}

void uf::allocator::deallocate( void* p, size_t n ) {
	if ( uf::allocator::useMemoryPool && uf::memoryPool::global.size() > 0 ) uf::memoryPool::global.free( p );
	else ::operator delete(p);
}