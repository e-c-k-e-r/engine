inline bool uf::aligned(const void* ptr, uintptr_t alignment) noexcept {
	return !( ((uintptr_t) ptr) % alignment );
}
template<class T> inline bool uf::aligned(const void* ptr) noexcept {
	return uf::aligned( ptr, alignof(T) );
}

inline uintptr_t uf::alignment(const void* ptr, uintptr_t a) noexcept {
	return ((uintptr_t) ptr) % a;
}
template<class T> inline uintptr_t uf::alignment(const void* ptr) noexcept {
	return uf::alignment( ptr, alignof(T) );
}

inline uint8_t* uf::realign(uint8_t* ptr, uintptr_t a) noexcept {
	const uintptr_t r = uf::alignment(ptr, a);
	if ( r != 0 ) ptr += a - r;
	return ptr;
}
template<class T> inline uint8_t* uf::realign(uint8_t* ptr) noexcept {
	return uf::realign( ptr, alignof(T) );
}