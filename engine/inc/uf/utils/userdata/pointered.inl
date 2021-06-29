namespace pod {
	struct UF_API PointeredUserdata {
		size_t len = 0;
	#if UF_USERDATA_RTTI
		size_t type = 0;
	#endif
		uint8_t* data = NULL;
	};
}

namespace uf {
	namespace pointeredUserdata {
		pod::PointeredUserdata UF_API create( std::size_t len, void* data = NULL );
		void UF_API destroy( pod::PointeredUserdata& userdata );
		pod::PointeredUserdata UF_API copy( const pod::PointeredUserdata& userdata );

		template<typename T> pod::PointeredUserdata create( const T& data = T() );
		template<typename T> T& get( pod::PointeredUserdata& userdata, bool validate = uf::userdata::autoValidate);
		template<typename T> const T& get(const pod::PointeredUserdata& userdata, bool validate = uf::userdata::autoValidate );
		template<typename T> bool is(const pod::PointeredUserdata& userdata);

		uf::stl::string UF_API toBase64( pod::PointeredUserdata& userdata );
		pod::PointeredUserdata UF_API fromBase64( const uf::stl::string& base64 );

	//	void move( pod::PointeredUserdata& to, pod::PointeredUserdata&& from );
	//	void copy( pod::PointeredUserdata& to, const pod::PointeredUserdata& from );

		// usage with MemoryPool
		pod::PointeredUserdata UF_API create( uf::MemoryPool&, std::size_t len, void* data = NULL );
		template<typename T> pod::PointeredUserdata create( uf::MemoryPool&, const T& data = T() );
		void UF_API destroy( uf::MemoryPool&, pod::PointeredUserdata& userdata );
		pod::PointeredUserdata UF_API copy( uf::MemoryPool&, const pod::PointeredUserdata& userdata );

		size_t UF_API size( size_t size, size_t padding = 0 );
	}
}

namespace uf {
	// Provides operations for POD Userdata
	class UF_API PointeredUserdata {
	public:
	// 	Easily access POD's type
		typedef pod::PointeredUserdata pod_t;
		bool autoDestruct = uf::userdata::autoDestruct;
	protected:
	// 	POD storage
		PointeredUserdata::pod_t m_pod = {};
	public:
	// 	C-tor
	
		PointeredUserdata(std::size_t len = 0, void* data = NULL); 	// initializes POD to default
		PointeredUserdata( const pod::PointeredUserdata& ); 						// initializes from POD
		PointeredUserdata( PointeredUserdata&& move ) noexcept; 					// Move c-tor
		PointeredUserdata( const PointeredUserdata& copy ); 				// Copy c-tor
	
		pod::PointeredUserdata& create( std::size_t len, void* data = NULL );
		void move( PointeredUserdata& move );
		void move( PointeredUserdata&& move );
		void copy( const PointeredUserdata& copy );
	// 	D-tor
		~PointeredUserdata() noexcept;
		void destroy();
	// 	POD access
		PointeredUserdata::pod_t& data(); 								// 	Returns a reference of POD
		const PointeredUserdata::pod_t& data() const; 					// 	Returns a const-reference of POD
		size_t size() const;
	#if UF_USERDATA_RTTI
		size_t type() const;
	#endif
	// 	Validity checks
		bool initialized() const;
	// 	Variadic construction
		template<typename T> pod::PointeredUserdata& create( const T& data = T() );
		template<typename T> T& get(bool = uf::userdata::autoValidate);
		template<typename T> const T& get(bool = uf::userdata::autoValidate) const;

		template<typename T> bool is() const;
		template<typename T> inline T& as() { return get<T>(); }
		template<typename T> inline const T& as() const { return get<T>(); }
	// 	Overloaded ops
		operator void*();
		operator void*() const;
		
		operator bool() const;
	
		PointeredUserdata& operator=( pod::PointeredUserdata pointer );
		PointeredUserdata& operator=( PointeredUserdata&& move );
		PointeredUserdata& operator=( const PointeredUserdata& copy );
	};
}

#include <cstdlib> // malloc, free

// Allows copy via assignment!
template<typename T>
pod::PointeredUserdata uf::pointeredUserdata::create( const T& data ) {
	return uf::pointeredUserdata::create<T>( uf::userdata::memoryPool, data );
}
template<typename T>
pod::PointeredUserdata uf::pointeredUserdata::create( uf::MemoryPool& requestedMemoryPool, const T& data ) {
//	uf::MemoryPool& memoryPool = uf::memoryPool::global.size() > 0 ? uf::memoryPool::global : requestedMemoryPool;
	size_t len = sizeof(data);
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::MemoryPool& memoryPool = requestedMemoryPool.size() > 0 ? requestedMemoryPool : uf::memoryPool::global;
	pod::PointeredUserdata userdata;
	userdata.len = len;
	userdata.data = (uint8_t*) memoryPool.alloc( nullptr, len );
#else
	uf::MemoryPool* memoryPool = NULL;
	if ( requestedMemoryPool.size() > 0 ) memoryPool = &requestedMemoryPool;
	else if ( uf::memoryPool::global.size() > 0 ) memoryPool = &uf::memoryPool::global;
	pod::PointeredUserdata userdata;
	userdata.len = len;
	userdata.data = NULL;
	if ( memoryPool )
		userdata.data = (uint8_t*) memoryPool->alloc( nullptr, len );
	else
		userdata.data = (uint8_t*) operator new( len ); 	// allocate data for the userdata struct, and then some	}
#endif
	userdata.len = len;
#if UF_USERDATA_RTTI
	userdata.type = typeid(T).hash_code();
#endif
	union {
		uint8_t* from;
		T* to;
	} kludge;
	kludge.from = userdata.data;
	::new (kludge.to) T(data);
	return userdata;
}
// Easy way to get the userdata as a reference
#include <stdexcept>
template<typename T>
T& uf::pointeredUserdata::get( pod::PointeredUserdata& userdata, bool validate ) {
	if ( validate && !uf::pointeredUserdata::is<T>( userdata ) ) UF_EXCEPTION("PointeredUserdata size|type mismatch: Expecting {" << typeid(T).hash_code() << ", " << sizeof(T) << "}, got {" << userdata.type << ", " << userdata.len << "}" );
	union {
		uint8_t* original;
		T* casted;
	} cast;
	cast.original = userdata.data;
	return *cast.casted;
}
template<typename T>
const T& uf::pointeredUserdata::get( const pod::PointeredUserdata& userdata, bool validate ) {
	if ( validate && !uf::pointeredUserdata::is<T>( userdata ) ) UF_EXCEPTION("PointeredUserdata size|type mismatch: Expecting {" << typeid(T).hash_code() << ", " << sizeof(T) << "}, got {" << userdata.type << ", " << userdata.len << "}" );
	union {
		const uint8_t* original;
		const T* casted;
	} cast;
	cast.original = userdata.data;
	return *cast.casted;
}

template<typename T>
bool uf::pointeredUserdata::is( const pod::PointeredUserdata& userdata ) {
#if UF_USERDATA_RTTI
	if ( userdata.type ) return userdata.type == typeid(T).hash_code() && userdata.len == sizeof(T);
#endif
	return userdata.len == sizeof(T);
}

// No need to cast data to a pointer AND get the data's size!
template<typename T>
pod::PointeredUserdata& uf::PointeredUserdata::create( const T& data ) {
	this->destroy();
	return this->m_pod = uf::pointeredUserdata::create(data);
}
// Easy way to get the userdata as a reference
template<typename T>
T& uf::PointeredUserdata::get(bool validate) {
	return uf::pointeredUserdata::get<T>( this->m_pod, validate );
}
// Easy way to get the userdata as a const-reference
template<typename T>
const T& uf::PointeredUserdata::get(bool validate) const {
	return uf::pointeredUserdata::get<T>( this->m_pod, validate );
}

template<typename T>
bool uf::PointeredUserdata::is() const {
	return uf::pointeredUserdata::is<T>( this->m_pod );
}