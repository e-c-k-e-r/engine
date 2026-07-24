#include <uf/utils/memory/allocator.h>
#include <uf/utils/memory/pool.h>
#include <iostream>

bool uf::allocator::override = true;

void* uf::allocator::allocate( size_t n ) {
	if ( override && uf::memoryPool::global.size() > 0 ) {
		return uf::memoryPool::global.alloc( n );
	}
	return std::malloc( n );
}

void uf::allocator::deallocate( void* p, size_t n ) {
	if ( !p ) return;

	if ( override && uf::memoryPool::global.size() > 0 ) {
		uf::memoryPool::global.free( p, n );
	}
	else std::free( p );
}

void* uf::allocator::malloc_m( size_t n ) {
	return std::malloc( n );
}

void uf::allocator::free_m( void* p, size_t n ) {
	std::free( p );
}

#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE
void* operator new( size_t n ) {
	void* p = uf::allocator::malloc_m( n );
	if ( !p ) throw std::bad_alloc();
	return p;
}

void operator delete( void* p ) noexcept {
	uf::allocator::free_m( p, 0 );
}

void* operator new[]( size_t n ) {
	void* p = uf::allocator::malloc_m( n );
	if ( !p ) throw std::bad_alloc();
	return p;
}

void operator delete[]( void* p ) noexcept {
	uf::allocator::free_m( p, 0 );
}

void operator delete( void* p, size_t n ) noexcept {
	uf::allocator::free_m( p, n );
}

void operator delete[]( void* p, size_t n ) noexcept {
	uf::allocator::free_m( p, n );
}
#endif