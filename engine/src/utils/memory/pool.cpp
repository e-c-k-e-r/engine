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
#define UF_MEMORYPOOL_LAZY 0

#define UF_MEMORYPOOL_CACHED_ALLOCATIONS 0
#define UF_MEMORYPOOL_STORE_ORPHANS 0

#define DEBUG_PRINT 0
#if DEBUG_PRINT
	#define UF_MSG_CONDITIONAL_PRINT(...) UF_MSG_DEBUG(__VA_ARGS__)
#else
	#define UF_MSG_CONDITIONAL_PRINT(...)
#endif

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

	uf::stl::stringstream ss; ss << std::hex << (void*) pool.memory;

	metadata["size"] = size;
	metadata["used"] = allocated;
	metadata["free"] = size - allocated;
	metadata["objects"] = pool.allocations.size();
	metadata["pool"] = ss.str();

	return metadata;
}
void uf::memoryPool::initialize( pod::MemoryPool& pool, size_t size ) {
	if ( size <= 0 ) return;
	if ( uf::memoryPool::size( pool ) > 0 ) uf::memoryPool::destroy( pool );

	pool.allocations.reserve(64);

	if ( uf::memoryPool::subPool && uf::memoryPool::global.size() > 0 && &pool != &uf::memoryPool::global.data() ) {
		pool.memory = uf::memoryPool::global.alloc( size );
	} else {
		pool.memory = uf::allocator::malloc_m( size );
	}
	UF_ASSERT( pool.memory );
	memset( pool.memory, 0, size );
	
	pool.size = size;
}
void uf::memoryPool::destroy( pod::MemoryPool& pool ) {
	if ( uf::memoryPool::size( pool ) <= 0 ) goto CLEAR;
	if ( uf::memoryPool::subPool && &pool != &uf::memoryPool::global.data() ) {
		uf::memoryPool::global.free( pool.memory );
	} else {
		uf::allocator::free_m(pool.memory);
	}
CLEAR:
	pool.size = 0;
	pool.memory = NULL;
}

pod::Allocation uf::memoryPool::allocate( pod::MemoryPool& pool, size_t size, size_t alignment ) {
//	alignment = MIN( alignment, uf::memoryPool::alignment );
//	alignment = uf::memoryPool::alignment;
#if UF_MEMORYPOOL_MUTEX
	pool.mutex.lock();
#endif
	// find next available allocation
	pod::Allocation allocation;

	uintptr_t pointer = (uintptr_t) pool.memory;
	// realign pointer
	if ( 0 < alignment ) {
		uintptr_t a = uf::alignment( (void*) pointer, alignment );
		pointer += a == 0 ? 0 : alignment - a;
	}

	size_t len = uf::memoryPool::size( pool );
	size_t padding = 0;
	// pool not initialized
	if ( len <= 0 ) {
		UF_MSG_CONDITIONAL_PRINT("cannot malloc, pool not initialized: " << size << " bytes");
		goto MANUAL_MALLOC;
	}

#if UF_MEMORYPOOL_CACHED_ALLOCATIONS
	// an optimization by quickly reusing freed allocations seemed like a good idea
	// but still have to iterate through allocation information
	// to keep allocation information in order

	// check our cache of first
	if ( !pool.cachedFreeAllocations.empty() ) {
		auto it = pool.cachedFreeAllocations.begin();
		// check if any recently freed allocation is big enough for our new allocation
		for ( ; it != pool.cachedFreeAllocations.end(); ++it ) {
			// is aligned
			if ( 0 < alignment && !uf::aligned( (void*) it->pointer, alignment ) ) continue;
			// sized adequately
			if ( it->size < size ) break;
		}
		// found a suitable allocation, use it
		if ( it != pool.cachedFreeAllocations.end() ) {
			pointer = it->pointer;
			// find where to insert in our allocation table
			auto next = pool.allocations.begin();
			while ( next != pool.allocations.end() ) {
				if ( pointer + size + padding < next->pointer  ) break;
				++next;
			}
			// check if it was actually valid
			if ( next != pool.allocations.end() ) {	
				// remove from cache
				pool.cachedFreeAllocations.erase(it);

				// initialize allocation info
				void* p = (void*) pointer;
				allocation.size = size;
				allocation.pointer = pointer;
				
				// security
			//	if ( data ) memcpy( p, data, size );
			//	else memset( p, 0, size );
				memset( p, 0, size );

				// register as allocated
				pool.allocations.insert(next, allocation);

				goto RETURN;
			}
		}
	}
#endif
	{
		// find any availble spots in-between existing allocations
		auto next = pool.allocations.begin();
		// beginning is big enough to fit
		if ( pointer + size < next->pointer ) {
		} else {
			for ( auto it = next; it != pool.allocations.end(); ++it ) {
				// ignore invalid indexes
				if ( pool.size == 0 ) continue;
				// point to end of allocation we're looking at
				pointer = it->pointer + it->size + padding;
				// realign our index if requested
				if ( 0 < alignment ) {
					uintptr_t a = uf::alignment( (void*) pointer, alignment );
					pointer += a == 0 ? 0 : alignment - a;
				}
				// hit the end of our allocations, break
				if ( ++next == pool.allocations.end() ) break;
				// target index is behind next allocated space, use it
				if ( pointer + size < next->pointer ) break;
			}
		}
		// no allocation found, OOM
		if ( (uintptr_t) pool.memory + len <= pointer + size + padding ) {
			UF_MSG_ERROR("MemoryPool: " << &pool << ": Out of Memory!");
			UF_MSG_ERROR("Trying to request " << size << " bytes of memory");
			UF_MSG_ERROR("Stats: " << uf::memoryPool::stats( pool ));
			goto MANUAL_MALLOC;
		}

		// initialize allocation info
		void* p = (void*) pointer;
		allocation.size = size;
		allocation.pointer = pointer;
		
		// security
	//	if ( data ) memcpy( p, data, size );
	//	else memset( p, 0, size );
		memset( p, 0, size );

		// overrides if we're overloading global new/delete
		// register as allocated
		pool.allocations.insert(next, allocation);
	}
	goto RETURN;
MANUAL_MALLOC:
#if UF_MEMORYPOOL_INVALID_MALLOC
	allocation.size = 0;
	allocation.pointer = (uintptr_t) uf::allocator::malloc_m(size);
#if UF_MEMORYPOOL_STORE_ORPHANS
	pool.orphaned.emplace_back(allocation);
#endif
#else
	UF_EXCEPTION("invalid malloc");
#endif
RETURN:
#if UF_MEMORYPOOL_MUTEX
	pool.mutex.unlock();
#endif
	UF_MSG_CONDITIONAL_PRINT((uintptr_t) allocation.pointer - (uintptr_t) pool.memory << " -> " << (uintptr_t) allocation.pointer + allocation.size - (uintptr_t) pool.memory - 1 );
	UF_ASSERT(allocation.pointer);
	return allocation;
}
void* uf::memoryPool::alloc( pod::MemoryPool& pool, size_t size, size_t alignment ) {
	auto allocation = uf::memoryPool::allocate( pool, size, alignment );
	return (void*) allocation.pointer;
}

pod::Allocation& uf::memoryPool::fetch( pod::MemoryPool& pool, void* pointer, size_t size ) {
	static pod::Allocation missing;
#if UF_MEMORYPOOL_FETCH_STL_FIND
	auto it = std::find_if( pool.allocations.begin(), pool.allocations.end(), [pointer, size]( const pod::Allocation& a ){
		return (uintptr_t) pointer == a.pointer && ((size > 0 && a.size == size) || (size == 0));
	});
	return it != pool.allocations.end() ? *it : missing;
#else
	for ( auto& allocation : pool.allocations ) {
		if ( (uintptr_t) pointer == allocation.pointer && ((size > 0 && allocation.size == size) || (size == 0)) ) return allocation;
	}
	return missing;
#endif
}
bool uf::memoryPool::exists( pod::MemoryPool& pool, void* pointer, size_t size ) {
	// bound check
#if UF_MEMORYPOOL_LAZY
	// if pointer lies before the start of the pool, or if it lies after the end of the pool
	return pool.memory <= pointer && pointer < (void*) ((uintptr_t) pool.memory + pool.size);
#else
	if ( !(pool.memory <= pointer && pointer < (void*) ((uintptr_t) pool.memory + pool.size)) ) return false;
	auto& allocation = uf::memoryPool::fetch( pool, pointer, size );
	return allocation.pointer == (uintptr_t) pointer && ((size > 0 && allocation.size == size) || (size == 0));
#endif
}
bool uf::memoryPool::free( pod::MemoryPool& pool, void* pointer, size_t size ) {
	// skip freeing, we're already a deallocated pool
	// this comes up because of how backasswards C++ static initialization/destruction order is
	if ( !pool.memory ) return false;
	// passed a NULL, for some reason
	if ( !pointer ) return false;
#if UF_MEMORYPOOL_MUTEX
	pool.mutex.lock();
#endif
	bool oob = pool.size <= 0 && !(pool.memory <= pointer && pointer < (void*) ((uintptr_t) pool.memory + pool.size));
#if UF_MEMORYPOOL_INVALID_MALLOC && UF_MEMORYPOOL_STORE_ORPHANS
	// if pointer is out of bounds
	if ( oob ) {
		// check if our pointer was an orphaned one
	#if UF_MEMORYPOOL_FETCH_STL_FIND
		auto it = std::find_if( pool.orphaned.begin(), pool.orphaned.end(), [pointer, size]( const pod::Allocation& a ){
			return (uintptr_t) pointer == a.pointer && ((size > 0 && a.size == size) || (size == 0));
		});
	#else
		auto it = pool.orphaned.begin();
		for ( ; it != pool.orphaned.end(); ++it ) if ( pointer == it->pointer && ((size > 0 && it->size == size) || (size == 0)) ) break;
	#endif
		// orphaned pointer, just free it
		if ( it != pool.orphaned.end() ) {
			uf::allocator::free_m( pointer );
			pool.orphaned.erase(it);
		}
	#if UF_MEMORYPOOL_MUTEX
		pool.mutex.unlock();
	#endif
		return true;
	}
#endif
	// fail if uninitialized or pointer is outside of our pool
	if ( oob ) {
		UF_MSG_CONDITIONAL_PRINT("cannot free: " << pointer << " | " << (pool.size <= 0) << " " << (pointer < pool.memory) << " " << (pointer >= (void*) ((uintptr_t) pool.memory + pool.size)));
		goto MANUAL_FREE;
	}
	{
		// pointer arithmatic
		auto it = pool.allocations.begin();
		pod::Allocation allocation{};
		// find our allocation in the allocation pool
		for ( ; it != pool.allocations.end(); ++it ) {
			if ( it->pointer != (uintptr_t) pointer ) continue;
			allocation = *it;
			break;
		}
		// pointer isn't actually allocated
		if ( allocation.pointer != (uintptr_t) pointer ) {
			UF_MSG_ERROR("cannot free, allocation not found: " << pointer);
			goto MANUAL_FREE;
		}
		// size validation mismatch, do not free
		if (0 < size && allocation.size != size) {
			UF_MSG_ERROR("cannot free, mismatched sized: " << pointer << " (" << size << " != " << allocation.size << ")");
			goto MANUAL_FREE;
		}
		UF_MSG_CONDITIONAL_PRINT("    " << (uintptr_t) allocation.pointer - (uintptr_t) pool.memory << " -> " << (uintptr_t) allocation.pointer + allocation.size - (uintptr_t) pool.memory - 1 );
		// security
		memset( pointer, 0, size );

		// remove from our allocation table...
		pool.allocations.erase(it);
	#if UF_MEMORYPOOL_CACHED_ALLOCATIONS
		// ...but add it to our freed allocation cache
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
	uf::allocator::free_m(pointer);
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