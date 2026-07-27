#include <uf/utils/memory/allocator.h>
#include <uf/utils/memory/pool.h>

namespace impl {
	using alloc_func_t = void* (*)(size_t);
	using free_func_t  = void  (*)(void*, size_t);

	void* pool_alloc( size_t n ) {
		return uf::memoryPool::global.alloc( n );
	}
	void pool_free( void* p, size_t n ) {
		uf::memoryPool::global.free( p, n );
	}

#if UF_MEMORYPOOL_OVERRIDE_DEFAULT
	alloc_func_t alloc_fn = impl::pool_alloc;
	free_func_t free_fn = impl::pool_free;
#else
	alloc_func_t alloc_fn = uf::allocator::malloc_m;
	free_func_t free_fn = uf::allocator::free_m;
#endif
}

void uf::allocator::override( bool state ) {
	if ( uf::memoryPool::global.size() == 0 ) state = false;
	if ( state ) {
		impl::alloc_fn = impl::pool_alloc;
		impl::free_fn = impl::pool_free;
	} else {
		impl::alloc_fn = uf::allocator::malloc_m;
		impl::free_fn = uf::allocator::free_m;
	}
}

void* uf::allocator::allocate( size_t n ) {
	return impl::alloc_fn( n );
}

void uf::allocator::deallocate( void* p, size_t n ) {
	impl::free_fn( p, n );
}

void* uf::allocator::malloc_m( size_t n ) {
	void* p = std::malloc( n );
	if ( !p ) UF_EXCEPTION("bad alloc");
	return p;
}

void uf::allocator::free_m( void* p, size_t n ) {
	std::free( p );
}

#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE
void* operator new( size_t n ) {
	void* p = uf::allocator::malloc_m( n );
	return p;
}

void operator delete( void* p ) noexcept {
	uf::allocator::free_m( p, 0 );
}

void* operator new[]( size_t n ) {
	void* p = uf::allocator::malloc_m( n );
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