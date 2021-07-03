#include <uf/utils/memory/allocator.h>
#include <uf/utils/memory/pool.h>

#define UF_MEMORYPOOL_OVERRIDE_NEW_DELETE 0
bool uf::allocator::override = false;
void* uf::allocator::allocate( size_t n ) {
	return uf::memoryPool::global.size() > 0 && uf::allocator::override ? uf::memoryPool::global.alloc( n ) : malloc( n );
}

void uf::allocator::deallocate( void* p, size_t n ) {
	if ( uf::memoryPool::global.size() > 0 && uf::allocator::override ) uf::memoryPool::global.free( p );
	else free( p );
}


void* uf::allocator::malloc_m( size_t n ) {
	return std::malloc( n );
}
void uf::allocator::free_m( void* p, size_t n ) {
	std::free( p );
}

//
#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE
void* operator new( size_t n ) {
	return uf::allocator::allocate( n );
}
void operator delete( void* p ) {
	uf::allocator::deallocate( p );
}
#endif