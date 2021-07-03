#pragma once

#include <uf/config.h>
#include <new>
// #include <limits>
// #include <functional>

#define UF_MEMORYPOOL_USE_STL_ALLOCATOR 0

namespace uf {
	namespace allocator {
		extern UF_API bool override;

		void* UF_API allocate( size_t n );
		void UF_API deallocate( void* p, size_t = 0 );
		
		void* UF_API malloc_m( size_t n );
		void UF_API free_m( void* p, size_t = 0 );
	
	/*	
		template<typename T>
		struct Use : std::true_type {};

		template<typename ... Ts>
		struct Use<std::function<Ts...>> : std::false_type {};
	*/
	}


	template <class T>
	struct Allocator {
		typedef T value_type;

		Allocator () = default;
		template <class U> constexpr Allocator (const Allocator <U>&) noexcept {}

		T* allocate(size_t n) noexcept {
		//	n *= sizeof(T);
		//	if ( !uf::allocator::Use<T>::value ) return static_cast<T*>( uf::allocator::malloc_m( n ) );
		//	return static_cast<T*>( uf::allocator::allocate( n ) );
			return static_cast<T*>( uf::allocator::allocate( n * sizeof(T) ) );
		}
		void deallocate(T* p, size_t n) noexcept {
		//	if ( !uf::allocator::Use<T>::value ) return uf::allocator::free_m(p);
			uf::allocator::deallocate( p, n );
		}
	};

	template <class T, class U>
	bool operator==(const uf::Allocator <T>&, const uf::Allocator <U>&) { return true; }

	template <class T, class U>
	bool operator!=(const uf::Allocator <T>&, const uf::Allocator <U>&) { return false; }

	template <class T>
	struct Mallocator {
		typedef T value_type;

		Mallocator () = default;
		template <class U> constexpr Mallocator (const Mallocator <U>&) noexcept {}

		T* allocate(size_t n) noexcept {
		//	n *= sizeof(T);
			return static_cast<T*>( uf::allocator::malloc_m( n * sizeof(T) ) );
		}
		void deallocate(T* p, size_t n) noexcept {
			uf::allocator::free_m( p, n );
		}
	};
	
	template <class T, class U>
	bool operator==(const uf::Mallocator <T>&, const uf::Mallocator <U>&) { return true; }

	template <class T, class U>
	bool operator!=(const uf::Mallocator <T>&, const uf::Mallocator <U>&) { return false; }
}


/*
	template <class T>
	class Allocator {
	public:
		using value_type    		= T;

//		template <class U> struct rebind {typedef Allocator<U> other;};
//		using pointer       		= value_type*;
//		using const_pointer 		= typename std::pointer_traits<pointer>::template rebind<value_type const>;
//		using void_pointer       	= typename std::pointer_traits<pointer>::template rebind<void>;
//		using const_void_pointer 	= typename std::pointer_traits<pointer>::template rebind<const void>;
//		using difference_type 		= typename std::pointer_traits<pointer>::difference_type;
//		using size_type       		= std::make_unsigned_t<difference_type>;

		Allocator() noexcept {}  // not required, unless used
		template <class U> Allocator(Allocator<U> const&) noexcept {}

		value_type* allocate( size_t n );
		void deallocate( value_type* p, size_t = 0 ) noexcept;

//     value_type*
//     allocate(size_t n, const_void_pointer) {
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

//		using propagate_on_container_copy_assignment = std::false_type;
//		using propagate_on_container_move_assignment = std::false_type;
//		using propagate_on_container_swap            = std::false_type;
//		using is_always_equal                        = std::is_empty<Allocator>;
		
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

namespace uf {
	template <class T>
	class Mallocator {
	public:
		using value_type    		= T;

//		template <class U> struct rebind {typedef Mallocator<U> other;};
//		using pointer       		= value_type*;
//		using const_pointer 		= typename std::pointer_traits<pointer>::template rebind<value_type const>;
//		using void_pointer       	= typename std::pointer_traits<pointer>::template rebind<void>;
//		using const_void_pointer 	= typename std::pointer_traits<pointer>::template rebind<const void>;
//		using difference_type 		= typename std::pointer_traits<pointer>::difference_type;
//		using size_type       		= std::make_unsigned_t<difference_type>;

		Mallocator() noexcept {}  // not required, unless used
		template <class U> Mallocator(Mallocator<U> const&) noexcept {}

		value_type* allocate( size_t n );
		void deallocate( value_type* p, size_t = 0 ) noexcept;

//     value_type*
//     allocate(size_t n, const_void_pointer) {
//         return allocate(n);
//     }

//     template <class U, class ...Args> void construct(U* p, Args&& ...args) {
//         ::new(p) U(std::forward<Args>(args)...);
//     }

//     template <class U> void destroy(U* p) noexcept {
//         p->~U();
//     }

//		size_t max_size() const noexcept { return std::numeric_limits<size_type>::max(); }
//		Mallocator select_on_container_copy_construction() const { return *this; }

//		using propagate_on_container_copy_assignment = std::false_type;
//		using propagate_on_container_move_assignment = std::false_type;
//		using propagate_on_container_swap            = std::false_type;
//		using is_always_equal                        = std::is_empty<Mallocator>;
		
	};

	template <class T, class U>
	bool operator==(uf::Mallocator<T> const&, uf::Mallocator<U> const&) noexcept {
		return true;
	}

	template <class T, class U>
	bool operator!=(uf::Mallocator<T> const& x, uf::Mallocator<U> const& y) noexcept {
		return !(x == y);
	}
}

#include "allocator.inl"
*/