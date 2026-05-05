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

#define UF_MEMORYPOOL_TEST 0

namespace {
	inline size_t getBucketIndex( size_t size, size_t currentSize, size_t levels = UF_MAX_BUCKETS - 1 ) {
		size_t index = 0;
		while ( currentSize < size && index < levels ) {
			currentSize <<= 1;
			index++;
		}
		return index;
	}
	inline size_t getTargetLevel( size_t size, size_t& currentSize, size_t levels = UF_MAX_BUCKETS - 1 ) {
		size_t index = 0;
		while ( currentSize < size && index < levels ) {
			currentSize <<= 1;
			index++;
		}
		return index;
	}

	inline void* getBuddy(void* block, size_t blockSize, void* poolStart) {
		uintptr_t offset = (uintptr_t)block - (uintptr_t) poolStart; // offset from start of the pool
		uintptr_t buddyOffset = offset ^ blockSize; // XOR the offset with the block size to find the buddy's offset
		return (void*)((uintptr_t)poolStart + buddyOffset);
	}

	inline size_t getTreeNodeIndex(size_t offset, size_t level, size_t maxLevel, size_t minChunkSize) {
		size_t indexInLevel = offset / (minChunkSize << level);
		size_t firstIndexOfLevel = (1 << (maxLevel - level)) - 1;
		return firstIndexOfLevel + indexInLevel;
	}

	inline void setBit(uint8_t* bitset, size_t index) {
		bitset[index / 8] |= (1 << (index % 8));
	}
	inline void clearBit(uint8_t* bitset, size_t index) {
		bitset[index / 8] &= ~(1 << (index % 8));
	}
	inline bool getBit(uint8_t* bitset, size_t index) {
		return (bitset[index / 8] & (1 << (index % 8))) != 0;
	}
}

bool uf::memoryPool::subPool = true;
uint8_t uf::memoryPool::alignment = 64;
uf::MemoryPool uf::memoryPool::global;

void uf::memoryPool::initialize( pod::MemoryPool& pool, size_t size, pod::MemoryPool::Strategy strategy, size_t chunkSize ) {
	if ( size <= 0 ) return;
	if ( uf::memoryPool::size( pool ) > 0 ) uf::memoryPool::destroy( pool );

	pool.size = size;
	pool.strategy = strategy;
	if ( uf::memoryPool::subPool && &pool != &uf::memoryPool::global.data() ) {
		pool.memory = uf::memoryPool::global.alloc( size );
	} else {
		pool.memory = uf::allocator::malloc_m( size );
	}
	UF_ASSERT( pool.memory );

	switch ( pool.strategy ) {
		case pod::MemoryPool::Strategy::LINEAR: {
			pool.state.linear.offset = 0;
			break;
		}
		case pod::MemoryPool::Strategy::POOL: {
			UF_ASSERT(chunkSize > 0 && chunkSize >= sizeof(void*));

			pool.state.pool.fixedChunkSize = chunkSize;
			pool.state.pool.freeListHead = pool.memory;

			size_t numChunks = size / chunkSize;
			uint8_t* ptr = static_cast<uint8_t*>(pool.memory);
			for (size_t i = 0; i < numChunks - 1; ++i) {
				void** currentChunk = reinterpret_cast<void**>(ptr + (i * chunkSize));
				*currentChunk = ptr + ((i + 1) * chunkSize);
			}
			void** lastChunk = reinterpret_cast<void**>(ptr + ((numChunks - 1) * chunkSize));
			*lastChunk = nullptr;
			break;
		}
		case pod::MemoryPool::Strategy::SEGREGATED: {
			UF_ASSERT(chunkSize >= sizeof(void*));

			pool.state.segregated.minChunkSize = chunkSize;
			pool.state.segregated.offset = 0;
			for (int i = 0; i < UF_MAX_BUCKETS; ++i) {
				pool.state.segregated.freeListHeads[i] = nullptr;
			}
			break;
		}
		case pod::MemoryPool::Strategy::BUDDY: {
			// to-do: pick a better fallback
			if ( !chunkSize ) chunkSize = 128;

			UF_ASSERT(chunkSize >= sizeof(void*));
			UF_ASSERT((size & (size - 1)) == 0); // is power of 2

			pool.state.buddy.minBlockSize = chunkSize;
			for (int i = 0; i < 32; ++i) pool.state.buddy.freeLists[i] = nullptr;

			// calculate max level
			size_t s = size;
			size_t maxLevel = 0;
			while ( s > chunkSize ) { s >>= 1; maxLevel++; }
			pool.state.buddy.maxLevel = maxLevel;

			pool.state.buddy.freeLists[maxLevel] = pool.memory; // set highest level as one free block 

			// allocate a small bitset to track splits
			// to-do: make it at the end of the allocation instead
			size_t totalNodes = (1 << (maxLevel + 1)) - 1;
			size_t bitsetSize = (totalNodes + 7) / 8; // round up to nearest byte
			pool.state.buddy.splitBlockBitset = (uint8_t*) uf::allocator::malloc_m(bitsetSize);
			memset(pool.state.buddy.splitBlockBitset, 0, bitsetSize);
			break;
		}
		default: {
			UF_EXCEPTION("invalid strategy: {}", pool.strategy);
			break;
		}
	}
}
void uf::memoryPool::destroy( pod::MemoryPool& pool ) {
	if ( uf::memoryPool::size( pool ) <= 0 || !pool.memory ) goto CLEAR;
	if ( uf::memoryPool::subPool && &pool != &uf::memoryPool::global.data() ) {
		uf::memoryPool::global.free( pool.memory, pool.size );
	} else {
		uf::allocator::free_m( pool.memory, pool.size );
	}

	// per-pool destruction
	switch ( pool.strategy ) {
		case pod::MemoryPool::Strategy::BUDDY: {
			uf::allocator::free_m(pool.state.buddy.splitBlockBitset);
			pool.state.buddy.splitBlockBitset = NULL;
			break;
		}
	}
CLEAR:
	pool.size = 0;
	pool.used = 0;
	pool.memory = NULL;
}

pod::Allocation uf::memoryPool::allocate( pod::MemoryPool& pool, size_t size, size_t alignment ) {
	pod::Allocation alloc;
#if UF_MEMORYPOOL_MUTEX
	std::lock_guard<std::mutex> lock(pool.mutex);
#endif

	switch ( pool.strategy ) {
		case pod::MemoryPool::Strategy::LINEAR: {
			// get next free space
			uintptr_t currentPtr = (uintptr_t) pool.memory + pool.state.linear.offset;

			// realign
			size_t padding = 0;
			if ( alignment > 0 ) {
				uintptr_t a = uf::alignment((void*) currentPtr, alignment);
				padding = a == 0 ? 0 : alignment - a;
			}

			// oom
			if ( pool.state.linear.offset + padding + size > pool.size ) {
				goto MANUAL_MALLOC;
			}

			// allocate
			alloc.pointer = currentPtr + padding;
			alloc.size = size;

			// move offset forward
			pool.state.linear.offset += padding + size;
			goto RETURN;
		}
		case pod::MemoryPool::Strategy::POOL: {
			// oom
			if ( size > pool.state.pool.fixedChunkSize || pool.state.pool.freeListHead == nullptr ) {
				goto MANUAL_MALLOC;
			}

			// allocate
			alloc.pointer = (uintptr_t) pool.state.pool.freeListHead;
			alloc.size = pool.state.pool.fixedChunkSize;

			// move head to next free spot
			pool.state.pool.freeListHead = *reinterpret_cast<void**>(pool.state.pool.freeListHead); // ???
			goto RETURN;
		}
		case pod::MemoryPool::Strategy::SEGREGATED: {
			// find bucket level
			size_t bucketIdx = getBucketIndex(size, pool.state.segregated.minChunkSize);
			size_t bucketSize = pool.state.segregated.minChunkSize << bucketIdx;

			// search within the free list first
			if ( pool.state.segregated.freeListHeads[bucketIdx] != nullptr ) {
				alloc.pointer = (uintptr_t)pool.state.segregated.freeListHeads[bucketIdx];
				alloc.size = bucketSize;
				pool.state.segregated.freeListHeads[bucketIdx] = *reinterpret_cast<void**>(alloc.pointer);
				goto RETURN;
			}

			// oom
			if (pool.state.segregated.offset + bucketSize > pool.size) {
				goto MANUAL_MALLOC;
			}

			// allocate
			alloc.pointer = (uintptr_t) pool.memory + pool.state.segregated.offset;
			alloc.size = bucketSize;
			
			// move offset to next free spot
			pool.state.segregated.offset += bucketSize;
			goto RETURN;
		}
		case pod::MemoryPool::Strategy::BUDDY: {
			// realign
			size_t requestedSize = size;
			if ( alignment > 0 && size < alignment ) requestedSize = alignment;

			// find target level
			size_t currentSize = pool.state.buddy.minBlockSize;
			size_t targetLevel = getTargetLevel(requestedSize, currentSize, pool.state.buddy.maxLevel);

			// find a free block at target level or higher
			size_t allocLevel = targetLevel;
			while ( allocLevel <= pool.state.buddy.maxLevel && pool.state.buddy.freeLists[allocLevel] == nullptr ) {
				allocLevel++;
			}

			// oom
			if ( allocLevel > pool.state.buddy.maxLevel ) {
				goto MANUAL_MALLOC;
			}

			// pop the block from the higher level
			void* block = pool.state.buddy.freeLists[allocLevel];
			pool.state.buddy.freeLists[allocLevel] = *reinterpret_cast<void**>(block);

			// split downwards to the target level
			while (allocLevel > targetLevel) {
				// mark the current block as split before dropping down
				size_t offset = (uintptr_t) block - (uintptr_t) pool.memory;
				size_t nodeIndex = getTreeNodeIndex( offset, allocLevel, pool.state.buddy.maxLevel, pool.state.buddy.minBlockSize );
				setBit(pool.state.buddy.splitBlockBitset, nodeIndex);

				allocLevel--;
				size_t halfSize = pool.state.buddy.minBlockSize << allocLevel;

				void* buddy = (void*) ((uintptr_t) block + halfSize); // right half

				// push buddy to the free list of this lower level
				*reinterpret_cast<void**>(buddy) = pool.state.buddy.freeLists[allocLevel];
				pool.state.buddy.freeLists[allocLevel] = buddy;
			}

			// allocate
			alloc.pointer = (uintptr_t) block;
			alloc.size = currentSize;
			goto RETURN;
		}
		default: {
			UF_EXCEPTION("invalid strategy: {}", pool.strategy);
			break;
		}
	}
MANUAL_MALLOC:
#if UF_MEMORYPOOL_INVALID_MALLOC
	alloc.size = 0; // orphaned
	alloc.pointer = (uintptr_t) uf::allocator::malloc_m(size);
	UF_MSG_DEBUG("allocating orphan: {} bytes at {}", size, (void*) alloc.pointer);

#if UF_MEMORYPOOL_STORE_ORPHANS
	pool.orphaned.emplace_back( alloc );
#endif
#else
	UF_EXCEPTION("cannot malloc: {}", size );
#endif

RETURN:
	pool.used += alloc.size;
	UF_ASSERT(alloc.pointer);
	return alloc;
}

bool uf::memoryPool::free( pod::MemoryPool& pool, void* pointer, size_t size ) {
	if ( !pointer ) return false;
#if UF_MEMORYPOOL_MUTEX
	std::lock_guard<std::mutex> lock(pool.mutex);
#endif
	bool oob = !exists( pool, pointer, size );
	if ( oob ) goto MANUAL_FREE;

	switch ( pool.strategy ) {
		case pod::MemoryPool::Strategy::LINEAR: {
			UF_EXCEPTION("cannot free individual allocation");
			return false;
		}

		case pod::MemoryPool::Strategy::POOL: {
			void** chunk = reinterpret_cast<void**>(pointer);
			*chunk = pool.state.pool.freeListHead; // point freed chunk to current head
			pool.state.pool.freeListHead = pointer; // freed chunk is now the new head
			goto RETURN;
		}
		case pod::MemoryPool::Strategy::SEGREGATED: {
			UF_ASSERT(size > 0);
			size_t bucketIdx = getBucketIndex(size, pool.state.segregated.minChunkSize);

			void** chunk = reinterpret_cast<void**>(pointer);
			*chunk = pool.state.segregated.freeListHeads[bucketIdx]; // point freed chunk to current head
			pool.state.segregated.freeListHeads[bucketIdx] = pointer; // freed chunk is now the new head
			goto RETURN;
		}
		case pod::MemoryPool::Strategy::BUDDY: {
			UF_ASSERT( size > 0 );
			void* block = pointer;

			// attempt to merge with buddies
			size_t currentSize = pool.state.buddy.minBlockSize;
			size_t level = getTargetLevel(size, currentSize, pool.state.buddy.maxLevel);
			while ( level < pool.state.buddy.maxLevel ) {
				void* buddy = getBuddy(block, currentSize, pool.memory);

				// search for buddy in the current level's free list
				void** current = &pool.state.buddy.freeLists[level];
				bool buddyIsFree = false;

				while ( *current != nullptr ) {
					// buddy is free, remove from free list
					if ( *current == buddy ) {
						*current = *reinterpret_cast<void**>(buddy);
						buddyIsFree = true;
						break;
					}
					current = reinterpret_cast<void**>(*current);
				}

				// buddy is allocated or split, stop merging
				if ( !buddyIsFree ) break;

				// merge; new block pointer is the minimum of the two
				block = (block < buddy) ? block : buddy;
				currentSize <<= 1;
				level++;
			}

			// push final block onto the appropriate free list
			*reinterpret_cast<void**>(block) = pool.state.buddy.freeLists[level];
			pool.state.buddy.freeLists[level] = block;
			goto RETURN;
		}
		default: {
			UF_EXCEPTION("invalid strategy: {}", pool.strategy);
			break;
		}
	}
MANUAL_FREE:
#if UF_MEMORYPOOL_STORE_ORPHANS
	if ( oob ) {
		auto it = pool.orphaned.begin();
		for ( ; it != pool.orphaned.end(); ++it ) {
			if ( (uintptr_t) pointer == it->pointer && ((size > 0 && it->size == size) || (size == 0)) ) break;
		}

		if ( it != pool.orphaned.end() ) {
			UF_MSG_DEBUG("manually freeing orphan {}", pointer);
			uf::allocator::free_m( pointer );
			pool.orphaned.erase(it);
			return true;
		}
	}
#endif

#if UF_MEMORYPOOL_INVALID_FREE
	UF_MSG_DEBUG("memory pool {}: manually freeing {}", (void*) &pool, pointer );
	uf::allocator::free_m(pointer);
	return true;
#else
	UF_ASSERT("cannot free: {}", pointer);
	return false;
#endif

RETURN:
	pool.used -= size;
	return true;
}
size_t uf::memoryPool::size( const pod::MemoryPool& pool ) {
	return pool.size;
}
void* uf::memoryPool::alloc( pod::MemoryPool& pool, size_t size, size_t alignment ) {
	auto allocation = uf::memoryPool::allocate( pool, size, alignment );
	return (void*) allocation.pointer;
}
bool uf::memoryPool::exists( pod::MemoryPool& pool, void* pointer, size_t size ) {
	// if pointer lies before the start of the pool, or if it lies after the end of the pool
	return pool.memory <= pointer && pointer < (void*) ((uintptr_t) pool.memory + pool.size);
}

size_t uf::memoryPool::allocated( const pod::MemoryPool& pool ) {
	return pool.used;
}
uf::stl::string uf::memoryPool::stats( const pod::MemoryPool& pool ) {
	uf::Serializer metadata;
	metadata["size"] = pool.size;
	metadata["used"] = pool.used;
	metadata["free"] = pool.size - pool.used;
	metadata["strategy"] = (int) pool.strategy;

	uf::stl::stringstream ss; ss << std::hex << (void*) pool.memory;
	metadata["pool"] = ss.str();
	return metadata;
}
/*
pod::Allocation& uf::memoryPool::fetch( pod::MemoryPool& pool, void* pointer, size_t size ) {
	UF_EXCEPTION("unimplemented");
}
const pod::MemoryPool::allocations_t& uf::memoryPool::allocations( const pod::MemoryPool& pool ) {
	UF_EXCEPTION("unimplemented")
}
*/
//
uf::MemoryPool::MemoryPool( size_t size ) {
	if ( size > 0 ) this->initialize( size );
}
uf::MemoryPool::~MemoryPool( ) {
	this->destroy();
}

#if UF_MEMORYPOOL_TEST
	#include "tests.inl"
#endif