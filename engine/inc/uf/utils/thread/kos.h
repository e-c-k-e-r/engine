#if UF_ENV_DREAMCAST
#include <kos/cond.h>

namespace uf {
	namespace stl {
		template <typename T>
		class atomic {
		private:
			volatile T m_value;

		public:
			atomic() : m_value(static_cast<T>(0)) {}
			atomic(T val) : m_value(val) {}

			inline T load(std::memory_order = std::memory_order_seq_cst) const {
				return m_value;
			}

			inline void store(T val, std::memory_order = std::memory_order_seq_cst) {
				m_value = val;
			}

			inline T fetch_add(T arg, std::memory_order = std::memory_order_seq_cst) {
				return __atomic_fetch_add(&m_value, arg, __ATOMIC_SEQ_CST);
			}

			inline T fetch_sub(T arg, std::memory_order = std::memory_order_seq_cst) {
				return __atomic_fetch_sub(&m_value, arg, __ATOMIC_SEQ_CST);
			}

			inline T exchange(T val, std::memory_order = std::memory_order_seq_cst) {
				return __atomic_exchange_n(&m_value, val, __ATOMIC_SEQ_CST);
			}

			inline T operator++() { return fetch_add(1) + 1; }
			inline T operator++(int) { return fetch_add(1); }
			inline T operator--() { return fetch_sub(1) - 1; }
			inline T operator--(int) { return fetch_sub(1); }

			inline T operator+=(T arg) { return fetch_add(arg) + arg; }
			inline T operator-=(T arg) { return fetch_sub(arg) - arg; }

			inline operator T() const { return load(); }
			inline T operator=(T val) { store(val); return val; }

			atomic(const atomic&) = delete;
			atomic& operator=(const atomic&) = delete;
		};

		using atomic_bool = atomic<bool>;
		using atomic_int = atomic<int>;
		using atomic_uint = atomic<unsigned int>;
	}
}
#endif