#pragma once

#include <uf/config.h>

#include <uf/utils/mempool/mempool.h>

#include <stdint.h>
#include <cstddef>
#include <string>
#include <algorithm>
#include <typeindex>

#define UF_USERDATA_POOLED 0
#define UF_USERDATA_KLUDGE 0
#define UF_USERDATA_RTTI 1

namespace pod {
	struct UF_API Userdata {
		std::size_t len = 0;
	#if UF_USERDATA_RTTI
		std::size_t type;
	#endif
		uint8_t data[1];
	};
}

namespace uf {
	namespace userdata {
		extern UF_API uf::MemoryPool memoryPool;

		pod::Userdata* UF_API create( std::size_t len, void* data = NULL );
		void UF_API destroy( pod::Userdata* userdata );
		pod::Userdata* UF_API copy( const pod::Userdata* userdata );

		template<typename T> pod::Userdata* create( const T& data = T() );
		template<typename T> T& get( pod::Userdata* userdata, bool validate = true);
		template<typename T> const T& get(const pod::Userdata* userdata, bool validate = true );
		template<typename T> bool is(const pod::Userdata* userdata);

		std::string UF_API toBase64( pod::Userdata* userdata );
		pod::Userdata* UF_API fromBase64( const std::string& base64 );

	//	void move( pod::Userdata& to, pod::Userdata&& from );
	//	void copy( pod::Userdata& to, const pod::Userdata& from );

		// usage with MemoryPool
		pod::Userdata* UF_API create( uf::MemoryPool&, std::size_t len, void* data = NULL );
		template<typename T> pod::Userdata* create( uf::MemoryPool&, const T& data = T() );
		void UF_API destroy( uf::MemoryPool&, pod::Userdata* userdata );
		pod::Userdata* UF_API copy( uf::MemoryPool&, const pod::Userdata* userdata );

		size_t UF_API size( size_t size, size_t padding = 0 );
	}
}

namespace uf {
	// Provides operations for POD Userdata
	class UF_API Userdata {
	public:
	// 	Easily access POD's type
		typedef pod::Userdata pod_t;
	protected:
	// 	POD storage
		Userdata::pod_t* m_pod = NULL;
	public:
	// 	C-tor
	
		Userdata(std::size_t len = 0, void* data = NULL); 	// initializes POD to default
		Userdata( pod::Userdata* ); 						// initializes from POD
		Userdata( Userdata&& move ); 						// Move c-tor
		Userdata( const Userdata& copy ); 					// Copy c-tor
	
		pod::Userdata* create( std::size_t len, void* data = NULL );
		void move( Userdata& move );
		void move( Userdata&& move );
		void copy( const Userdata& copy );
	// 	D-tor
		~Userdata();
		void destroy();
	// 	POD access
		Userdata::pod_t& data(); 								// 	Returns a reference of POD
		const Userdata::pod_t& data() const; 					// 	Returns a const-reference of POD
	// 	Validity checks
		bool initialized() const;
	// 	Variadic construction
		template<typename T> pod::Userdata* create( const T& data = T() );
		template<typename T> T& get();
		template<typename T> const T& get() const;

		template<typename T> bool is() const;
		template<typename T> inline T& as() { return get<T>(); }
		template<typename T> inline const T& as() const { return get<T>(); }
	// 	Overloaded ops
		operator void*();
		operator void*() const;
		
		operator bool() const;
	
		Userdata& operator=( pod::Userdata* pointer );
		Userdata& operator=( Userdata&& move );
		Userdata& operator=( const Userdata& copy );
	
	};
}

#include "userdata.inl"