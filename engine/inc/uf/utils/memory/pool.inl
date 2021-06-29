template<typename T>
T& uf::memoryPool::alloc( pod::MemoryPool& pool, const T& data/*, size_t alignment*/ ) {
	auto allocation = uf::memoryPool::allocate( pool, nullptr, sizeof(data), uf::memoryPool::alignment == 0 ? alignof(T) : uf::memoryPool::alignment );
	union {
		uint8_t* from;
		T* to;
	} kludge;
	kludge.from = (uint8_t*) allocation.pointer;
	::new (kludge.to) T(data);
	return *kludge.to;
}
template<typename T>
pod::Allocation uf::memoryPool::allocate( pod::MemoryPool& pool, const T& data/*, size_t alignment*/ ) {
	auto allocation = uf::memoryPool::allocate( pool, nullptr, sizeof(data), uf::memoryPool::alignment == 0 ? alignof(T) : uf::memoryPool::alignment );
	if ( !allocation.pointer ) return allocation;
	union {
		uint8_t* from;
		T* to;
	} kludge;
	kludge.from = (uint8_t*) allocation.pointer;
	::new (kludge.to) T(data);
	return allocation;
}
template<typename T>
bool uf::memoryPool::exists( pod::MemoryPool& pool, const T& data ) {
	if ( std::is_pointer<T>::value ) return uf::memoryPool::exists( pool, (void*) data );
	return uf::memoryPool::exists( pool, (void*) &data, sizeof(data) );
//	return uf::memoryPool::exists( pool, (void*) (std::is_pointer<T>::value ? data : &data), sizeof(data) );
}
template<typename T>
bool uf::memoryPool::free( pod::MemoryPool& pool, const T& data ) {
	if ( std::is_pointer<T>::value ) return uf::memoryPool::free( pool, (void*) data );
	return uf::memoryPool::free( pool, (void*) &data, sizeof(data) );
//	return uf::memoryPool::free( pool, (void*) (std::is_pointer<T>::value ? data : &data), sizeof(data) );
}

size_t uf::MemoryPool::size() const { return uf::memoryPool::size( m_pod ); }
size_t uf::MemoryPool::allocated() const { return uf::memoryPool::allocated( m_pod ); }
uf::stl::string uf::MemoryPool::stats() const { return uf::memoryPool::stats( m_pod ); }
void uf::MemoryPool::initialize( size_t size ) { return uf::memoryPool::initialize( m_pod, size ); }
void uf::MemoryPool::destroy() { return uf::memoryPool::destroy( m_pod ); }

pod::Allocation uf::MemoryPool::allocate( void* data, size_t size/*, size_t alignment*/ ) { return uf::memoryPool::allocate( m_pod, data, size/*, alignment*/ ); }
void* uf::MemoryPool::alloc( void* data, size_t size/*, size_t alignment*/ ) { return uf::memoryPool::alloc( m_pod, data, size/*, alignment*/ ); }
void* uf::MemoryPool::alloc( size_t size, void* data/*, size_t alignment*/ ) { return uf::memoryPool::alloc( m_pod, data, size/*, alignment*/ ); }
pod::Allocation& uf::MemoryPool::fetch( size_t index, size_t size ) { return uf::memoryPool::fetch( m_pod, index, size ); }
bool uf::MemoryPool::exists( void* data, size_t size ) { return uf::memoryPool::exists( m_pod, data, size ); }
bool uf::MemoryPool::free( void* data, size_t size ) { return uf::memoryPool::free( m_pod, data, size ); }

const pod::MemoryPool::allocations_t& uf::MemoryPool::allocations() const { return uf::memoryPool::allocations( m_pod ); }
inline pod::MemoryPool& uf::MemoryPool::data() { return m_pod; }
inline const pod::MemoryPool& uf::MemoryPool::data() const { return m_pod; }

template<typename T> T& uf::MemoryPool::alloc( const T& data/*, size_t alignment*/ ) { return uf::memoryPool::alloc( m_pod, data/*, alignment*/ ); }
template<typename T> pod::Allocation uf::MemoryPool::allocate( const T& data/*, size_t alignment*/ ) { return uf::memoryPool::allocate( m_pod, data/*, alignment*/ ); }
template<typename T> bool uf::MemoryPool::exists( const T& data ) { return uf::memoryPool::exists( m_pod, data ); }
template<typename T> bool uf::MemoryPool::free( const T& data ) { return uf::memoryPool::free( m_pod, data ); }