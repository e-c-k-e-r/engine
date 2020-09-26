#pragma once

#include <uf/config.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <mutex>

#define UF_MEMORYPOOL_MUTEX 1
#define UF_MEMORYPOOL_INVALID_MALLOC 0
#define UF_MEMORYPOOL_INVALID_FREE 0
#define UF_MEMORYPOOL_OVERRIDE_NEW_DELETE 0

namespace pod {
	struct UF_API Userdata;
	struct UF_API Allocation {
		size_t index = 0;
		size_t size = 0;
		void* pointer = NULL;
	};
}

namespace uf {
	class UF_API MemoryPool {
	protected:
		size_t m_size;
		uint8_t* m_pool;
		std::mutex m_mutex;

		typedef std::vector<pod::Allocation> allocations_t;
		allocations_t m_allocations;
	public:
		static bool globalOverride;
		static bool subPool;
		static MemoryPool global;

		MemoryPool( size_t = 0 );
		~MemoryPool();

		size_t size() const;
		size_t allocated() const;
		std::string stats() const;
		void initialize( size_t );
		void destroy();

		pod::Allocation allocate( void*, size_t );
		void* alloc( void*, size_t );
		inline void* alloc( size_t size, void* data = NULL ) { return alloc(data, size); }
		pod::Allocation& fetch( size_t, size_t = 0 );
		bool exists( void*, size_t = 0 );
		bool free( void*, size_t = 0 );
		
		template<typename T> T& alloc( const T& = T() );
		template<typename T> pod::Allocation allocate( const T& = T() );
		template<typename T> bool exists( const T& = T() );
		template<typename T> bool free( const T& = T() );

		const allocations_t& allocations() const;
	/*
		struct Iterator {
			uf::MemoryPool& pool;
			pod::Allocation& allocation;
			pod::Allocation& next;
			pod::Allocation& prev;
		};
		Iterator begin();
		Iterator end();
	*/
	};
}

#include "mempool.inl"