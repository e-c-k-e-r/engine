#pragma once

#include <uf/config.h>
#include <stdint.h>
#include <vector>
#include <uf/utils/memory/string.h>
#include <mutex>

#define UF_MEMORYPOOL_INVALID_MALLOC 1
#define UF_MEMORYPOOL_INVALID_FREE 1

#define UF_MEMORYPOOL_CACHED_ALLOCATIONS 0
#define UF_MEMORYPOOL_STORE_ORPHANS 0

namespace pod {
	struct UF_API Userdata;

	struct UF_API Allocation {
		size_t size = 0;
		uintptr_t pointer = 0;
	};

	struct UF_API MemoryPool {
		typedef std::vector<pod::Allocation, uf::Mallocator<pod::Allocation>> allocations_t;

		size_t size = 0;
		void* memory = 0;

		std::mutex mutex;
		allocations_t allocations;

	#if UF_MEMORYPOOL_CACHED_ALLOCATIONS
		allocations_t cachedFreeAllocations;
	#endif
	#if UF_MEMORYPOOL_STORE_ORPHANS
		allocations_t orphaned;
	#endif
	};
}

namespace uf {
	namespace memoryPool {
		extern UF_API bool subPool;
		extern UF_API uint8_t alignment;

		size_t UF_API size( const pod::MemoryPool& );
		size_t UF_API allocated( const pod::MemoryPool& );
		uf::stl::string UF_API stats( const pod::MemoryPool& );
		void UF_API initialize( pod::MemoryPool&, size_t );
		void UF_API destroy( pod::MemoryPool& );

	//	pod::Allocation UF_API allocate( pod::MemoryPool&, void*, size_t, size_t alignment = uf::memoryPool::alignment );
	//	void* UF_API alloc( pod::MemoryPool&, void*, size_t, size_t alignment = uf::memoryPool::alignment );
	//	inline void* alloc( pod::MemoryPool& pool, size_t size, void* data = NULL, size_t alignment = uf::memoryPool::alignment ) { return uf::memoryPool::alloc(pool, data, size, alignment); }
		pod::Allocation UF_API allocate( pod::MemoryPool&, size_t, size_t alignment = uf::memoryPool::alignment );
		void* UF_API alloc( pod::MemoryPool&, size_t, size_t alignment = uf::memoryPool::alignment );

		pod::Allocation& UF_API fetch( pod::MemoryPool&, void*, size_t = 0 );
		bool UF_API exists( pod::MemoryPool&, void*, size_t = 0 );
		bool UF_API free( pod::MemoryPool&, void*, size_t = 0 );

		const pod::MemoryPool::allocations_t& UF_API allocations( const pod::MemoryPool& );
		
		template<typename T> T& alloc( pod::MemoryPool&, const T& = T()/*, size_t alignment = uf::memoryPool::alignment*/ );
		template<typename T> pod::Allocation allocate( pod::MemoryPool&, const T& = T()/*, size_t alignment = uf::memoryPool::alignment*/ );
		template<typename T> bool exists( pod::MemoryPool&, const T& = T() );
		template<typename T> bool free( pod::MemoryPool&, const T& = T() );
	}
}

namespace uf {
	class UF_API MemoryPool {
	protected:
		pod::MemoryPool m_pod;
	public:
		MemoryPool( size_t = 0 );
		~MemoryPool();

		inline size_t size() const;
		inline size_t allocated() const;
		inline uf::stl::string stats() const;
		inline void initialize( size_t size );
		inline void destroy();

	//	inline pod::Allocation allocate( void* data, size_t size/*, size_t alignment = uf::memoryPool::alignment*/ );
	//	inline void* alloc( void* data, size_t size/*, size_t alignment = uf::memoryPool::alignment*/ );
	//	inline void* alloc( size_t size, void* data = NULL/*, size_t alignment = uf::memoryPool::alignment*/ );
		inline pod::Allocation allocate( size_t size/*, size_t alignment = uf::memoryPool::alignment*/ );
		inline void* alloc( size_t size/*, size_t alignment = uf::memoryPool::alignment*/ );
		inline pod::Allocation& fetch( void* data, size_t size = 0 );
		inline bool exists( void* data, size_t size = 0 );
		inline bool free( void* data, size_t size = 0 );
		
		inline const pod::MemoryPool::allocations_t& allocations() const;
		inline pod::MemoryPool& data();
		inline const pod::MemoryPool& data() const;

		template<typename T> inline T& alloc( const T& data = T()/*, size_t alignment = uf::memoryPool::alignment*/ );
		template<typename T> inline pod::Allocation allocate( const T& data = T()/*, size_t alignment = uf::memoryPool::alignment*/ );
		template<typename T> inline bool exists( const T& data = T() );
		template<typename T> inline bool free( const T& data = T() );
	};
}

namespace uf {
	namespace memoryPool {
		extern UF_API uf::MemoryPool global;
	}
}

#include "pool.inl"