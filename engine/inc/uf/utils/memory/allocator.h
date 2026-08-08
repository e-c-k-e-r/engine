#pragma once

#include <uf/config.h>
#include <cstddef>
#include <new>
#include <type_traits>

#define UF_MEMORYPOOL_USE_ALLOCATOR 1
#define UF_MEMORYPOOL_OVERRIDE_DEFAULT 1
#if __clang__
	#define UF_MEMORYPOOL_OVERRIDE_NEW_DELETE 0
#else
	#define UF_MEMORYPOOL_OVERRIDE_NEW_DELETE  1
#endif

namespace uf {
	namespace allocator {
		void UF_API override( bool state );

		void* UF_API allocate( size_t n );
		void  UF_API deallocate( void* p, size_t n = 0 );

		void* UF_API malloc_m( size_t n );
		void  UF_API free_m( void* p, size_t n = 0 );
	}

	template <class T>
	struct Allocator {
		typedef T value_type;
		typedef std::true_type propagate_on_container_swap;
		typedef std::true_type propagate_on_container_move_assignment;
		typedef std::true_type is_always_equal;

		Allocator() = default;
		template <class U> constexpr Allocator( const Allocator<U>& ) noexcept {}

		T* allocate( size_t n ) {
			void* p = uf::allocator::allocate( n * sizeof(T) );
			return static_cast<T*>(p);
		}

		void deallocate( T* p, size_t n ) noexcept {
			uf::allocator::deallocate( p, n * sizeof(T) );
		}
	};

	template <class T, class U>
	bool operator==( const uf::Allocator<T>&, const uf::Allocator<U>& ) { return true; }

	template <class T, class U>
	bool operator!=( const uf::Allocator<T>&, const uf::Allocator<U>& ) { return false; }

	// will never ever use the pool
	template <class T>
	struct Mallocator {
		typedef T value_type;
		typedef std::true_type propagate_on_container_swap;
		typedef std::true_type propagate_on_container_move_assignment;
		typedef std::true_type is_always_equal;

		Mallocator() = default;
		template <class U> constexpr Mallocator( const Mallocator<U>& ) noexcept {}

		T* allocate( size_t n ) {
			void* p = uf::allocator::malloc_m( n * sizeof(T) );
			return static_cast<T*>( p );
		}

		void deallocate(T* p, size_t n) noexcept {
			uf::allocator::free_m( p, n * sizeof(T) );
		}
	};

	template <class T, class U>
	bool operator==(const uf::Mallocator<T>&, const uf::Mallocator<U>&) { return true; }

	template <class T, class U>
	bool operator!=(const uf::Mallocator<T>&, const uf::Mallocator<U>&) { return false; }
}