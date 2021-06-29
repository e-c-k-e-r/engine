#pragma once

#include <uf/config.h>

namespace uf {
	inline bool aligned(const void* ptr, uintptr_t alignment) noexcept;
	template<class T> inline bool aligned(const void* ptr) noexcept;

	inline uintptr_t alignment(const void* ptr, uintptr_t a) noexcept;
	template<class T> inline uintptr_t alignment(const void* ptr) noexcept;

	inline uint8_t* realign(uint8_t* ptr, uintptr_t a) noexcept;
	template<class T> inline uint8_t* realign(uint8_t* ptr) noexcept;
}

#include "alignment.inl"