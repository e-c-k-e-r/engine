#include <uf/utils/mempool/mempool.h>
#include <uf/utils/userdata/userdata.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/thread/perthread.h>
#include <uf/utils/io/iostream.h>

namespace {
	uf::ThreadUnique<bool> globalOverrides;
}

#define DEBUG_PRINT 0

bool uf::MemoryPool::globalOverride = true;
bool uf::MemoryPool::subPool = true;
uf::MemoryPool uf::MemoryPool::global;

uf::MemoryPool::MemoryPool( size_t size ) {
	if ( size > 0 ) this->initialize( size );
}
uf::MemoryPool::~MemoryPool( ) {
	this->destroy();
}
size_t uf::MemoryPool::size() const {
	return this->m_size;
}
size_t uf::MemoryPool::allocated() const {
	size_t allocated = 0;
	for ( auto& allocation : this->m_allocations ) {
		allocated += allocation.size;
	}
	return allocated;
}
std::string uf::MemoryPool::stats() const {
	uf::Serializer metadata;

	size_t size = this->size();
	size_t allocated = this->allocated();

	metadata["size"] = size;
	metadata["used"] = allocated;
	metadata["free"] = size - allocated;
	metadata["objects"] = this->m_allocations.size();
	{
		std::stringstream ss; ss << std::hex << (void*) this->m_pool;
		metadata["pool"] = ss.str();
	}

	return metadata;
}
void uf::MemoryPool::initialize( size_t size ) {
	if ( size <= 0 ) return;
	if ( this->size() > 0 ) this->destroy();
	this->m_size = size;
	if ( uf::MemoryPool::subPool && uf::MemoryPool::global.size() > 0 && this != &uf::MemoryPool::global ) {
		this->m_pool = (uint8_t*) uf::MemoryPool::global.alloc( NULL, size );
	} else {
		this->m_pool = (uint8_t*) malloc( size );
	}
	memset( this->m_pool, 0, size );
}
void uf::MemoryPool::destroy() {
	if ( this->size() <= 0 ) return;
	if ( uf::MemoryPool::subPool && this != &uf::MemoryPool::global ) {
		uf::MemoryPool::global.free( this->m_pool );
	} else {
		delete[] this->m_pool;
	}
	this->m_size = 0;
	this->m_pool = NULL;
}

pod::Allocation uf::MemoryPool::allocate( void* data, size_t size ) {
	if ( UF_MEMORYPOOL_MUTEX ) this->m_mutex.lock();
	// find next available allocation
	size_t index = 0;
	size_t len = this->size();
	pod::Allocation allocation;
	// pool not initialized
	if ( len <= 0 ) {
		if ( DEBUG_PRINT ) uf::iostream << "CANNOT MALLOC: " << size << ", POOL NOT INITIALIZED" << "\n";
		goto MANUAL_MALLOC;
	}

	// an optimization by quickly reusing free'd allocations seemed like a good idea
	// but still have to iterate through allocation information
	// to keep allocation information in order

	// check our cache of first
#if UF_MEMORYPOOL_CACHED_ALLOCATIONS
	if ( !this->m_cachedFreeAllocations.empty() ) {
		auto it = this->m_cachedFreeAllocations.begin();
		// check if any recently free'd allocation is big enough for our new allocation
		for ( ; it != this->m_cachedFreeAllocations.end(); ++it ) {
			if ( it->size < size ) break;
		}
		// found a suitable allocation, use it
		if ( it != this->m_cachedFreeAllocations.end() ) {
			index = it->index;
			// find where to insert in our allocation table
			auto next = this->m_allocations.begin();
			while ( next != this->m_allocations.end() ) {
				if ( index + size < next->index  ) break;
				++next;
			}
			// check if it was actually valid
			if ( next != this->m_allocations.end() ) {	
				// initialize allocation info
				allocation.index = index;
				allocation.size = size;
				allocation.pointer = &this->m_pool[0] + index;
				
				// security
				if ( data ) memcpy( allocation.pointer, data, size );
				else memset( allocation.pointer, 0, size );

				// overrides if we're overloading global new/delete
				globalOverrides.get() = true;
				// register as allocated
				this->m_allocations.insert(next, allocation);
				globalOverrides.get() = false;

				goto RETURN;
			}
			// remove from cache
			this->m_cachedFreeAllocations.erase(it);
		}
	}
#endif
	{
		// find any availble spots in-between existing allocations
		auto next = this->m_allocations.begin();
		for ( auto it = next; it != this->m_allocations.end(); ++it ) {
			index = it->index + it->size;
			if ( ++next == this->m_allocations.end() ) break;
			// target index is behind next allocated space, use it
			if ( index < next->index ) break;
		}
		// no allocation found, OOM
		if ( index + size > len ) {
			uf::iostream << "MemoryPool: " << this << ": Out of Memory!\n";
			uf::iostream << "Trying to request " << size << " bytes of memory\n";
			uf::iostream << "Stats: " << this->stats() << "\n";
			goto MANUAL_MALLOC;
		}

		// initialize allocation info
		allocation.index = index;
		allocation.size = size;
		allocation.pointer = &this->m_pool[0] + index;
		
		// security
		if ( data ) memcpy( allocation.pointer, data, size );
		else memset( allocation.pointer, 0, size );

		// overrides if we're overloading global new/delete
		globalOverrides.get() = true;
		// register as allocated
		this->m_allocations.insert(next, allocation);
		globalOverrides.get() = false;
	}
	goto RETURN;
MANUAL_MALLOC:
	if ( UF_MEMORYPOOL_INVALID_MALLOC ) {
		allocation.index = -1;
		allocation.size = size;
		allocation.pointer = malloc(size);
	} else {
		UF_EXCEPTION("INVALID MALLOC");
	}
RETURN:
	if ( UF_MEMORYPOOL_MUTEX ) this->m_mutex.unlock();
	if ( DEBUG_PRINT ) uf::iostream << "MALLOC'd: " << allocation.pointer << ", " << allocation.size << ", " << allocation.index << "\n";
	return allocation;
}
void* uf::MemoryPool::alloc( void* data, size_t size ) {
	auto allocation = this->allocate( data, size );
	return allocation.pointer;
}

pod::Allocation& uf::MemoryPool::fetch( size_t index, size_t size ) {
	static pod::Allocation missing;
	for ( auto& allocation : this->m_allocations ) {
		if ( index == allocation.index ) {
			if ( (size > 0 && allocation.size == size) || (size == 0) ) return allocation;
		}
	}
	return missing;
}
bool uf::MemoryPool::exists( void* pointer, size_t size ) {
	// bound check
	if ( pointer < &this->m_pool[0] ) return false;
	size_t index = (uint8_t*) pointer - &this->m_pool[0];
	// size check
	if ( size > 0 ) {
		auto& allocation = this->fetch( index, size );
		return allocation.index == index && allocation.size == size;
	}
	return true;
}
bool uf::MemoryPool::free( void* pointer, size_t size ) {
	if ( UF_MEMORYPOOL_MUTEX ) this->m_mutex.lock();
	// fail if uninitialized or pointer is outside of our pool
	if ( this->m_size <= 0 || pointer < &this->m_pool[0] || pointer >= &this->m_pool[0] + this->m_size ) {
		if ( DEBUG_PRINT ) uf::iostream << "CANNOT FREE: " << pointer << ", ERROR: " << (this->m_size <= 0) << " " << (pointer < &this->m_pool[0]) << " " << (pointer >= &this->m_pool[0] + this->m_size) << "\n";
		goto MANUAL_FREE;
	}
	{
		// pointer arithmatic
		size_t index = (uint8_t*) pointer - &this->m_pool[0];
		auto it = this->m_allocations.begin();
		pod::Allocation allocation;
		// find our allocation in the allocation pool
		for ( ; it != this->m_allocations.end(); ++it ) {
			if ( it->index == index ) {
				allocation = *it;
				break;
			}
		}
		// pointer isn't actually allocated
		if ( allocation.index != index ) {
			if ( DEBUG_PRINT ) uf::iostream << "CANNOT FREE: " << pointer << ", NOT FOUND" << "\n";
			goto MANUAL_FREE;
		}
		// size validation mismatch, do not free
		if (size > 0 && allocation.size != size) {
			if ( DEBUG_PRINT ) uf::iostream << "CANNOT FREE: " << pointer << ", MISMATCHED SIZES (" << size << " != " << allocation.size << ")" << "\n";
			goto MANUAL_FREE;
		}
		if ( DEBUG_PRINT ) uf::iostream << "FREE'D ALLOCATION: " << pointer << ", " << size << "\t" << allocation.pointer << ", " << allocation.size << ", " << allocation.index << "\n";
		// remove from our allocation table...
		this->m_allocations.erase(it);
		// ...but add it to our free'd allocation cache
#if UF_MEMORYPOOL_CACHED_ALLOCATIONS
		this->m_cachedFreeAllocations.push_back(allocation);
#endif
		if ( UF_MEMORYPOOL_MUTEX ) this->m_mutex.unlock();
		return true;
	}
MANUAL_FREE:
	if ( UF_MEMORYPOOL_MUTEX ) this->m_mutex.unlock();
	if ( UF_MEMORYPOOL_INVALID_FREE ) ::free(pointer);
	return false;
}

const uf::MemoryPool::allocations_t& uf::MemoryPool::allocations() const {
	return this->m_allocations;
}
#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE
void* operator new( size_t size ) {
	if ( !uf::MemoryPool::globalOverride || globalOverrides.get() || uf::MemoryPool::global.size() <= 0 )
		return malloc(size);
	return uf::MemoryPool::global.alloc( (void*) NULL, size );
}
void operator delete( void* pointer ) {
	if ( !uf::MemoryPool::globalOverride ) return ::free( pointer );
	uf::MemoryPool::global.free( pointer );
}
#endif
/*
uf::MemoryPool::Iterator uf::MemoryPool::begin() {
	static pod::Allocation end;
	end = {
		.index = 0
		.size = 0,
		.pointer = this->m_pool
	};
	struct {
		pod::Allocation* begin = NULL;
		pod::Allocation* end = NULL;
		pod::Allocation* next = NULL;
	} pointers;
	pointers.begin = &end;
	pointers.end = &end;
	pointers.prev = &end;

	if ( this->m_allocations.size() > 0 ) {
		auto it = this->m_allocations.begin();
		pointers.begin = &(*it++);
		if ( it != this->m_allocations.end() )
			pointers.next = &(*it);
	}

	return {
		.pool = *this,
		.allocation = *pointers.begin,
		.prev = *pointers.end,
		.next = *pointers.next
	};
}
uf::MemoryPool::Iterator uf::MemoryPool::end() {
	static pod::Allocation end;
	end = {
		.index = this->m_size,
		.size = this->m_size,
		.pointer = this->m_pool + this->m_size;
	}
	struct {
		pod::Allocation* end = NULL;
		pod::Allocation* prev = NULL;
	} pointers;
	
	if ( this->m_allocations.size() > 0 ) {
		auto it = this->m_allocations.begin();
		pointers.end = &(*it++);
		if ( it != this->m_allocations.end() )
			pointers.prev = &(*it);
	}
	pointers.prev = &end;

	return {
		.pool = *this,
		.allocation = *pointers.end,
		.prev = *pointers.prev,
		.next = *pointers.end
	};
}
*/