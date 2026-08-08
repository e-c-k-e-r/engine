#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <vector>

namespace uf {
	namespace stl {
		template<
			class T,
		#if UF_MEMORYPOOL_USE_ALLOCATOR
			class Allocator = std::allocator<T>
		#else
			class Allocator = uf::Allocator<T>
		#endif
		>
		using vector = std::vector<T, Allocator>;

		template<typename T>
		T& random( uf::stl::vector<T>& v ) {
			return v[rand() % v.size()];		
		}

		template<typename T>
		T random_it( T begin, T end ) {
			uf::stl::vector<T> its;
			for ( auto it = begin; it != end; ++it ) its.emplace_back( it );
			return random( its );
		}

		// a little overkill
		template <typename T, size_t N = 12>
		class static_vector {
		private:
			T m_data[N];
			size_t m_size = 0;

		public:
			static_vector() = default;
			static_vector(std::initializer_list<T> list) {
				for (const auto& item : list) {
					if (m_size >= N) break;
					m_data[m_size++] = item;
				}
			}
			static_vector(const static_vector& other) : m_size(other.m_size) {
				for (size_t i = 0; i < m_size; ++i) {
					m_data[i] = other.m_data[i];
				}
			}
			static_vector& operator=(const static_vector& other) {
				if (this != &other) {
					m_size = other.m_size;
					for (size_t i = 0; i < m_size; ++i) {
						m_data[i] = other.m_data[i];
					}
				}
				return *this;
			}

			static_vector(static_vector&& other) noexcept : m_size(other.m_size) {
				for (size_t i = 0; i < m_size; ++i) {
					m_data[i] = std::move(other.m_data[i]);
				}
				other.m_size = 0;
			}

			static_vector& operator=(static_vector&& other) noexcept {
				if (this != &other) {
					m_size = other.m_size;
					for (size_t i = 0; i < m_size; ++i) {
						m_data[i] = std::move(other.m_data[i]);
					}
					other.m_size = 0;
				}
				return *this;
			}

			inline T& push_back(const T& val) {
				if (m_size < N) {
					m_data[m_size] = val;
					return m_data[m_size++];
				}
				return m_data[N - 1];
			}

			template <typename... Args>
			inline T& emplace_back(Args&&... args) {
				if (m_size < N) {
					m_data[m_size] = T(std::forward<Args>(args)...);
					return m_data[m_size++];
				}
				return m_data[N - 1];
			}

			inline T* erase(T* pos) {
				T* first = begin();
				T* last = end();

				if ( pos >= first && pos < last ) {
					size_t index = pos - first;

					for ( size_t i = index; i < m_size - 1; ++i ) {
						m_data[i] = std::move(m_data[i + 1]);
					}
					--m_size;
				}
				return pos;
			}

			inline T& operator[](size_t index) { return m_data[index]; }
			inline const T& operator[](size_t index) const { return m_data[index]; }

			inline T* begin() { return m_data; }
			inline const T* begin() const { return m_data; }
			inline T* end() { return m_data + m_size; }
			inline const T* end() const { return m_data + m_size; }

			inline T& front() { return m_data[0]; }
			inline const T& front() const { return m_data[0]; }

			inline T& back() { return m_data[m_size - 1]; }
			inline const T& back() const { return m_data[m_size - 1]; }

			inline size_t size() const { return m_size; }
			inline bool empty() const { return m_size == 0; }
			inline void clear() { m_size = 0; }
			inline void reserve(size_t) {}
			inline void resize(size_t s) {
				if (s <= N) m_size = s;
			}
		};
	}
}