#pragma once

#include <uf/config.h>

#define UF_MEMORYPOOL_OVERRIDE_NEW_DELETE 1
namespace uf {
	namespace allocator {
		extern UF_API bool useMemoryPool;
		void* UF_API allocate( size_t n );
		void UF_API deallocate( void* p, size_t = 0 );
	}

	template <class T>
	class Allocator {
	public:
	    using value_type    		= T;
	
		template <class U> struct rebind {typedef Allocator<U> other;};
		using pointer       		= value_type*;
		using const_pointer 		= typename std::pointer_traits<pointer>::template rebind<value_type const>;
		using void_pointer       	= typename std::pointer_traits<pointer>::template rebind<void>;
		using const_void_pointer 	= typename std::pointer_traits<pointer>::template rebind<const void>;
		using difference_type 		= typename std::pointer_traits<pointer>::difference_type;
		using size_type       		= std::make_unsigned_t<difference_type>;

		Allocator() noexcept {}  // not required, unless used
		template <class U> Allocator(Allocator<U> const&) noexcept {}

		value_type* allocate( size_t n );
		void deallocate( value_type* p, size_t = 0 ) noexcept;

//     value_type*
//     allocate(std::size_t n, const_void_pointer) {
//         return allocate(n);
//     }

//     template <class U, class ...Args> void construct(U* p, Args&& ...args) {
//         ::new(p) U(std::forward<Args>(args)...);
//     }

//     template <class U> void destroy(U* p) noexcept {
//         p->~U();
//     }

//		size_t max_size() const noexcept { return std::numeric_limits<size_type>::max(); }
//		Allocator select_on_container_copy_construction() const { return *this; }

		using propagate_on_container_copy_assignment = std::false_type;
		using propagate_on_container_move_assignment = std::false_type;
		using propagate_on_container_swap            = std::false_type;
		using is_always_equal                        = std::is_empty<Allocator>;
		
	};

	template <class T, class U>
	bool operator==(uf::Allocator<T> const&, uf::Allocator<U> const&) noexcept {
	    return true;
	}

	template <class T, class U>
	bool operator!=(uf::Allocator<T> const& x, uf::Allocator<U> const& y) noexcept {
	    return !(x == y);
	}
}


#include "allocator.inl"