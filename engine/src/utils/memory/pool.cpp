#include <uf/utils/memory/pool.h>
#include <uf/utils/memory/alignment.h>
#include <uf/utils/memory/allocator.h>
#include <uf/utils/userdata/userdata.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/thread/perthread.h>
#include <uf/utils/io/iostream.h>

#define UF_MEMORYPOOL_MUTEX 1
#define UF_MEMORYPOOL_FETCH_STL_FIND 1
#define UF_MEMORYPOOL_LAZY 1
#define UF_MEMORYPOOL_CACHED_ALLOCATIONS 0
#define UF_MEMORYPOOL_OVERRIDE_NEW_DELETE_DEPRECATED 0

#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE_DEPRECATED
namespace {
	uf::ThreadUnique<bool> globalOverrides;
}
#endif

#define DEBUG_PRINT 0
#if DEBUG_PRINT
	#define UF_MSG_CONDITIONAL_PRINT(...) UF_MSG_DEBUG(__VA_ARGS__)
#else
	#define UF_MSG_CONDITIONAL_PRINT(...)
#endif

bool uf::memoryPool::globalOverride = true;
bool uf::memoryPool::subPool = true;
uint8_t uf::memoryPool::alignment = 64;
uf::MemoryPool uf::memoryPool::global;

size_t uf::memoryPool::size( const pod::MemoryPool& pool ) {
	return pool.size;
}
size_t uf::memoryPool::allocated( const pod::MemoryPool& pool ) {
	size_t allocated = 0;
	for ( auto& allocation : pool.allocations ) allocated += allocation.size;
	return allocated;
}
uf::stl::string uf::memoryPool::stats( const pod::MemoryPool& pool ) {
	uf::Serializer metadata;

	size_t size = uf::memoryPool::size( pool );
	size_t allocated = uf::memoryPool::allocated( pool );

	metadata["size"] = size;
	metadata["used"] = allocated;
	metadata["free"] = size - allocated;
	metadata["objects"] = pool.allocations.size();
	{
		uf::stl::stringstream ss; ss << std::hex << (void*) pool.pool;
		metadata["pool"] = ss.str();
	}

	return metadata;
}
void uf::memoryPool::initialize( pod::MemoryPool& pool, size_t size ) {
	if ( size <= 0 ) return;

	if ( uf::memoryPool::size( pool ) > 0 ) uf::memoryPool::destroy( pool );
	pool.size = size;

	if ( uf::memoryPool::subPool && uf::memoryPool::global.size() > 0 && &pool != &uf::memoryPool::global.data() ) {
		pool.pool = (uint8_t*) uf::memoryPool::global.alloc( nullptr, size );
	} else {
		pool.pool = (uint8_t*) malloc( size );
	}
	UF_ASSERT( pool.pool );
	memset( pool.pool, 0, size );
}
void uf::memoryPool::destroy( pod::MemoryPool& pool ) {
	if ( uf::memoryPool::size( pool ) <= 0 ) return;
	if ( uf::memoryPool::subPool && &pool != &uf::memoryPool::global.data() ) {
		uf::memoryPool::global.free( pool.pool );
	} else {
		delete[] pool.pool;
	}
	pool.size = 0;
	pool.pool = NULL;
}

pod::Allocation uf::memoryPool::allocate( pod::MemoryPool& pool, void* data, size_t size, size_t alignment ) {
//	alignment = MIN( alignment, uf::memoryPool::alignment );
//	alignment = uf::memoryPool::alignment;
#if UF_MEMORYPOOL_MUTEX
	pool.mutex.lock();
#endif
	// find next available allocation
	size_t index = 0;
	size_t len = uf::memoryPool::size( pool );
	pod::Allocation allocation;
	// pool not initialized
	if ( len <= 0 ) {
		UF_MSG_ERROR("cannot malloc, pool not initialized: " << size);
		goto MANUAL_MALLOC;
	}

	// an optimization by quickly reusing freed allocations seemed like a good idea
	// but still have to iterate through allocation information
	// to keep allocation information in order

	// check our cache of first
#if UF_MEMORYPOOL_CACHED_ALLOCATIONS
	if ( !pool.cachedFreeAllocations.empty() ) {
		auto it = pool.cachedFreeAllocations.begin();
		// check if any recently freed allocation is big enough for our new allocation
		for ( ; it != pool.cachedFreeAllocations.end(); ++it ) {
			// is aligned
			if ( 0 < alignment && !uf::aligned( &pool.pool[0] + it->index, alignment ) ) continue;
			// sized adequately
			if ( it->size < size ) break;
		}
		// found a suitable allocation, use it
		if ( it != pool.cachedFreeAllocations.end() ) {
			index = it->index;
			// find where to insert in our allocation table
			auto next = pool.allocations.begin();
			while ( next != pool.allocations.end() ) {
				if ( index + size < next->index  ) break;
				++next;
			}
			// check if it was actually valid
			if ( next != pool.allocations.end() ) {	
				// remove from cache
				pool.cachedFreeAllocations.erase(it);

				// initialize allocation info
				allocation.index = index;
				allocation.size = size;
				allocation.pointer = &pool.pool[0] + index;
				
				// security
				if ( data ) memcpy( allocation.pointer, data, size );
				else memset( allocation.pointer, 0, size );

				// overrides if we're overloading global new/delete
			#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE_DEPRECATED
				globalOverrides.get() = true;
			#endif
				// register as allocated
				pool.allocations.insert(next, allocation);
			#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE_DEPRECATED
				globalOverrides.get() = false;
			#endif

				goto RETURN;
			}
		}
	}
#endif
	{
		// find any availble spots in-between existing allocations
		auto next = pool.allocations.begin();
		size_t alignedSize = size;
		for ( auto it = next; it != pool.allocations.end(); ++it ) {
			// point to end of allocation
			index = it->index + it->size;
			// realign our index if requested
			if ( 0 < alignment ) {
				uintptr_t a = uf::alignment( &pool.pool[0] + index, alignment );
				uintptr_t o = a == 0 ? 0 : alignment - a;
				index += o;
			}
			// hit the end of our allocations, break
			if ( ++next == pool.allocations.end() ) break;
			// target index is behind next allocated space, use it
			if ( index < next->index ) break;
		}
		// no allocation found, OOM
		if ( index + size > len ) {
			UF_MSG_ERROR("MemoryPool: " << &pool << ": Out of Memory!");
			UF_MSG_ERROR("Trying to request " << size << " bytes of memory");
			UF_MSG_ERROR("Stats: " << uf::memoryPool::stats( pool ));
			goto MANUAL_MALLOC;
		}

		// initialize allocation info
		allocation.index = index;
		allocation.size = size;
		allocation.pointer = &pool.pool[0] + index;
		
		// security
		if ( data ) memcpy( allocation.pointer, data, size );
		else memset( allocation.pointer, 0, size );

		// overrides if we're overloading global new/delete
	#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE_DEPRECATED
		globalOverrides.get() = true;
	#endif
		// register as allocated
		pool.allocations.insert(next, allocation);
	#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE_DEPRECATED
		globalOverrides.get() = false;
	#endif
	}
	goto RETURN;
MANUAL_MALLOC:
#if UF_MEMORYPOOL_INVALID_MALLOC
	allocation.index = -1;
	allocation.size = size;
	allocation.pointer = malloc(size);
#else
	UF_EXCEPTION("invalid malloc");
#endif
RETURN:
#if UF_MEMORYPOOL_MUTEX
	pool.mutex.unlock();
#endif
	UF_MSG_CONDITIONAL_PRINT("malloc'd: " << allocation.pointer << ", " << allocation.size << ", " << allocation.index);
//	UF_ASSERT(allocation.pointer);
	return allocation;
}
void* uf::memoryPool::alloc( pod::MemoryPool& pool, void* data, size_t size, size_t alignment ) {
	auto allocation = uf::memoryPool::allocate( pool, data, size, alignment );
	return allocation.pointer;
}

pod::Allocation& uf::memoryPool::fetch( pod::MemoryPool& pool, size_t index, size_t size ) {
	static pod::Allocation missing;
#if UF_MEMORYPOOL_FETCH_STL_FIND
	auto it = std::find_if( pool.allocations.begin(), pool.allocations.end(), [index, size]( const pod::Allocation& a ){
		return index == a.index && ((size > 0 && a.size == size) || (size == 0));
	});
	return it != pool.allocations.end() ? *it : missing;
#else
	for ( auto& allocation : pool.allocations ) {
		if ( index == allocation.index && ((size > 0 && allocation.size == size) || (size == 0)) ) return allocation;
	}
	return missing;
#endif
}
bool uf::memoryPool::exists( pod::MemoryPool& pool, void* pointer, size_t size ) {
	// bound check
#if UF_MEMORYPOOL_LAZY
//	return &pool.pool[0] <= pointer && &pool.pool[0] + pool.size < pointer + size;
	if ( pointer < &pool.pool[0] ) return false;
	if ( &pool.pool[0] + pool.size < pointer - size ) return false;
	return true;
#else
//	if ( !(&pool.pool[0] <= pointer && &pool.pool[0] + pool.size < pointer + size) ) return false;
	if ( pointer < &pool.pool[0] ) return false;
	if ( &pool.pool[0] + pool.size < pointer - size ) return false;
	size_t index = (uint8_t*) pointer - &pool.pool[0];
	// size check
	auto& allocation = uf::memoryPool::fetch( pool, index, size );
	return allocation.index == index && ((size > 0 && allocation.size == size) || (size == 0));
#endif
}
bool uf::memoryPool::free( pod::MemoryPool& pool, void* pointer, size_t size ) {
	// passed a NULL, for some reason
	if ( !pointer ) return false;
#if UF_MEMORYPOOL_MUTEX
	pool.mutex.lock();
#endif
	// fail if uninitialized or pointer is outside of our pool
	if ( pool.size <= 0 || pointer < &pool.pool[0] || pointer >= &pool.pool[0] + pool.size ) {
		UF_MSG_ERROR("cannot free: " << pointer << " | " << (pool.size <= 0) << " " << (pointer < &pool.pool[0]) << " " << (pointer >= &pool.pool[0] + pool.size));
		goto MANUAL_FREE;
	}
	{
		// pointer arithmatic
		size_t index = (uint8_t*) pointer - &pool.pool[0];
		auto it = pool.allocations.begin();
		pod::Allocation allocation;
		// find our allocation in the allocation pool
		for ( ; it != pool.allocations.end(); ++it ) {
			if ( it->index != index ) continue;
			allocation = *it;
			break;
		}
		// pointer isn't actually allocated
		if ( allocation.index != index ) {
			UF_MSG_ERROR("cannot free, allocation not found: " << pointer);
			goto MANUAL_FREE;
		}
		// size validation mismatch, do not free
		if (size > 0 && allocation.size != size) {
			UF_MSG_ERROR("cannot free, mismatched sized: " << pointer << " (" << size << " != " << allocation.size << ")");
			goto MANUAL_FREE;
		}
		UF_MSG_CONDITIONAL_PRINT("freed allocation: " << pointer << ", " << size << "\t" << allocation.pointer << ", " << allocation.size << ", " << allocation.index);
		// remove from our allocation table...
		pool.allocations.erase(it);
		// ...but add it to our freed allocation cache
	#if UF_MEMORYPOOL_CACHED_ALLOCATIONS
		pool.cachedFreeAllocations.push_back(allocation);
	#endif
	#if UF_MEMORYPOOL_MUTEX
		pool.mutex.unlock();
	#endif
		return true;
	}
MANUAL_FREE:
#if UF_MEMORYPOOL_MUTEX
	pool.mutex.unlock();
#endif
#if UF_MEMORYPOOL_INVALID_FREE
	::free(pointer);
#endif
	return false;
}

const pod::MemoryPool::allocations_t& uf::memoryPool::allocations( const pod::MemoryPool& pool ) {
	return pool.allocations;
}
//
uf::MemoryPool::MemoryPool( size_t size ) {
	if ( size > 0 ) this->initialize( size );
}
uf::MemoryPool::~MemoryPool( ) {
	this->destroy();
}
//
#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE
void* operator new( size_t n ) {
	return uf::allocator::useMemoryPool && uf::memoryPool::global.size() > 0 ? uf::memoryPool::global.alloc( nullptr, n ) : malloc( n );
}
void operator delete( void* p ) {
	if ( uf::allocator::useMemoryPool && uf::memoryPool::global.size() > 0 ) uf::memoryPool::global.free( p );
	else free(p);
}
#endif