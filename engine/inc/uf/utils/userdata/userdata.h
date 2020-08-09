#pragma once

#include <uf/config.h>
#include <stdint.h>
#include <cstddef>
#include <string>
#include <algorithm>

namespace pod {
	struct UF_API Userdata {
		std::size_t len = 0;
		uint8_t data[1];
	};
}

namespace uf {
	namespace userdata {
		pod::Userdata* UF_API create( std::size_t len, void* data );
		void UF_API destroy( pod::Userdata* userdata );

		template<typename T> pod::Userdata* create( const T& data = T() );
		template<typename T> T& get( pod::Userdata* userdata, bool validate = true);
		template<typename T> const T& get(const pod::Userdata* userdata);

		std::string UF_API toBase64( pod::Userdata* userdata );
		pod::Userdata* UF_API fromBase64( const std::string& base64 );

	//	void move( pod::Userdata& to, pod::Userdata&& from );
	//	void copy( pod::Userdata& to, const pod::Userdata& from );
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
	
		Userdata(std::size_t len = 0, void* data = NULL); 	// initializes POD to 'def'
		Userdata( Userdata&& move ); 						// Move c-tor
		Userdata( const Userdata& copy ); 					// Copy c-tor
	
		pod::Userdata* create( std::size_t len, void* data );
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
	// 	Overloaded ops
		operator void*();
		operator void*() const;
		
		operator bool() const;
	
		Userdata& operator=( Userdata&& move );
		Userdata& operator=( const Userdata& copy );
	
	};
}

#include "userdata.inl"