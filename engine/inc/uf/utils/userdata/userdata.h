#pragma once

#include <uf/config.h>

#include <uf/utils/memory/pool.h>

#include <stdint.h>
#include <cstddef>
#include <uf/utils/memory/string.h>
#include <algorithm>
#include <typeindex>

#define UF_USERDATA_POOLED 0
#define UF_USERDATA_KLUDGE 0

#define UF_USERDATA_CTTI_TYPE TYPE_HASH_T // size_t
#define UF_USERDATA_CTTI(T) TYPE_HASH(T)

#define DEBUG_PRINT_TYPE(name, T) UF_MSG_DEBUG(name << ": " << TYPE_NAME(T) << " | " << TYPE_HASH(T) << " | " << sizeof(T) );

namespace pod {
	struct UF_API Userdata {
		size_t len = 0;
		UF_USERDATA_CTTI_TYPE type = UF_USERDATA_CTTI(void);
		uint8_t data[1];
	};
}

namespace uf {
	namespace userdata {
		extern UF_API uf::MemoryPool memoryPool;
		extern UF_API bool autoDestruct;
		extern UF_API bool autoValidate;

		pod::Userdata* UF_API create( size_t len, void* data = NULL );
		void UF_API destroy( pod::Userdata* userdata );
		pod::Userdata* UF_API copy( const pod::Userdata* userdata );

		template<typename T> pod::Userdata* create( const T& data = T() );
		template<typename T> T& get( pod::Userdata* userdata, bool validate = uf::userdata::autoValidate);
		template<typename T> const T& get(const pod::Userdata* userdata, bool validate = uf::userdata::autoValidate );
		template<typename T> bool is(const pod::Userdata* userdata);

		uf::stl::string UF_API toBase64( pod::Userdata* userdata );
		pod::Userdata* UF_API fromBase64( const uf::stl::string& base64 );

	//	void move( pod::Userdata& to, pod::Userdata&& from );
	//	void copy( pod::Userdata& to, const pod::Userdata& from );

		// usage with MemoryPool
		pod::Userdata* UF_API create( uf::MemoryPool&, size_t len, void* data = NULL );
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
		bool autoDestruct = uf::userdata::autoDestruct;
	protected:
	// 	POD storage
		Userdata::pod_t* m_pod = NULL;
	public:
	// 	C-tor
	
		Userdata( size_t len = 0, void* data = NULL ); 	// initializes POD to default
		Userdata( pod::Userdata* ); 						// initializes from POD
		Userdata( Userdata&& move ) noexcept ; 				// Move c-tor
		Userdata( const Userdata& copy ); 		// Copy c-tor
	
		pod::Userdata* create( size_t len, void* data = NULL );
		void move( Userdata& move );
		void move( Userdata&& move );
		void copy( const Userdata& copy );
	// 	D-tor
		~Userdata() noexcept;
		void destroy();
	// 	POD access
		Userdata::pod_t& data(); 								// 	Returns a reference of POD
		const Userdata::pod_t& data() const; 					// 	Returns a const-reference of POD
		size_t size() const;
		UF_USERDATA_CTTI_TYPE type() const;
		// 	Validity checks
		bool initialized() const;
	// 	Variadic construction
		template<typename T> pod::Userdata* create( const T& data = T() );
		template<typename T> T& get(bool = uf::userdata::autoValidate);
		template<typename T> const T& get(bool = uf::userdata::autoValidate) const;

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
#include "pointered.inl"