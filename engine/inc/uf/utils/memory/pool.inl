template<typename T>
T& uf::memoryPool::alloc( pod::MemoryPool& pool, const T& data/*, size_t alignment*/ ) {
	auto allocation = uf::memoryPool::allocate( pool, sizeof(data), uf::memoryPool::alignment == 0 ? alignof(T) : uf::memoryPool::alignment );
	union {
		void* from;
		T* to;
	} kludge;
	kludge.from = (void*) allocation.pointer;
	::new (kludge.to) T(data);
	return *kludge.to;
}
template<typename T>
pod::Allocation uf::memoryPool::allocate( pod::MemoryPool& pool, const T& data/*, size_t alignment*/ ) {
	auto allocation = uf::memoryPool::allocate( pool, sizeof(data), uf::memoryPool::alignment == 0 ? alignof(T) : uf::memoryPool::alignment );
	if ( !allocation.pointer ) return allocation;
	union {
		void* from;
		T* to;
	} kludge;
	kludge.from = (void*) allocation.pointer;
	::new (kludge.to) T(data);
	return allocation;
}
template<typename T>
bool uf::memoryPool::exists( pod::MemoryPool& pool, const T& data ) {
	return std::is_pointer<T>::value ? uf::memoryPool::exists( pool, (void*) data ) : uf::memoryPool::exists( pool, (void*) &data, sizeof(data) );
}
template<typename T>
bool uf::memoryPool::free( pod::MemoryPool& pool, const T& data ) {
#if __cplusplus >= 201703L
	if constexpr (std::is_pointer_v<T>) {
		return uf::memoryPool::free( pool, (void*) data, sizeof(std::remove_pointer_t<T>) );
	} else {
		return uf::memoryPool::free( pool, (void*) &data, sizeof(T) );
	}
#else
	return std::is_pointer<T>::value
		? uf::memoryPool::free( pool, (void*) data, sizeof(typename std::remove_pointer<T>::type) )
		: uf::memoryPool::free( pool, (void*) &data, sizeof(T) );
#endif
}

size_t uf::MemoryPool::size() const { return uf::memoryPool::size( *this ); }
size_t uf::MemoryPool::allocated() const { return uf::memoryPool::allocated( *this ); }
uf::stl::string uf::MemoryPool::stats() const { return uf::memoryPool::stats( *this ); }
void uf::MemoryPool::initialize( size_t size, pod::MemoryPool::Strategy strategy, size_t chunkSize ) { return uf::memoryPool::initialize( *this, size, strategy, chunkSize ); }
void uf::MemoryPool::destroy() { return uf::memoryPool::destroy( *this ); }

//pod::Allocation uf::MemoryPool::allocate( void* data, size_t size/*, size_t alignment*/ ) { return uf::memoryPool::allocate( *this, data, size/*, alignment*/ ); }
//void* uf::MemoryPool::alloc( void* data, size_t size/*, size_t alignment*/ ) { return uf::memoryPool::alloc( *this, data, size/*, alignment*/ ); }
//void* uf::MemoryPool::alloc( size_t size, void* data/*, size_t alignment*/ ) { return uf::memoryPool::alloc( *this, data, size/*, alignment*/ ); }
pod::Allocation uf::MemoryPool::allocate( size_t size/*, size_t alignment*/ ) { return uf::memoryPool::allocate( *this, size/*, alignment*/ ); }
void* uf::MemoryPool::alloc( size_t size/*, size_t alignment*/ ) { return uf::memoryPool::alloc( *this, size/*, alignment*/ ); }
//pod::Allocation& uf::MemoryPool::fetch( void* data, size_t size ) { return uf::memoryPool::fetch( *this, data, size ); }
bool uf::MemoryPool::exists( void* data, size_t size ) { return uf::memoryPool::exists( *this, data, size ); }
bool uf::MemoryPool::free( void* data, size_t size ) { return uf::memoryPool::free( *this, data, size ); }

//const pod::MemoryPool::allocations_t& uf::MemoryPool::allocations() const { return uf::memoryPool::allocations( *this ); }
inline pod::MemoryPool& uf::MemoryPool::data() { return *this; }
inline const pod::MemoryPool& uf::MemoryPool::data() const { return *this; }

template<typename T> T& uf::MemoryPool::alloc( const T& data/*, size_t alignment*/ ) { return uf::memoryPool::alloc( *this, data/*, alignment*/ ); }
template<typename T> pod::Allocation uf::MemoryPool::allocate( const T& data/*, size_t alignment*/ ) { return uf::memoryPool::allocate( *this, data/*, alignment*/ ); }
template<typename T> bool uf::MemoryPool::exists( const T& data ) { return uf::memoryPool::exists( *this, data ); }
template<typename T> bool uf::MemoryPool::free( const T& data ) { return uf::memoryPool::free( *this, data ); }