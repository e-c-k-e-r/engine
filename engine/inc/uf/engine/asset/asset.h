#pragma once

#include <uf/utils/http/http.h>
#include <uf/engine/entity/entity.h>
#include <uf/utils/resolvable/resolvable.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/memory/unordered_map.h>
#include <functional>

#include "payload.h"

namespace uf {
	namespace asset {
		struct UF_API Job {
			typedef uf::stl::vector<Job> container_t;

			uf::stl::string callback = "";
			uf::stl::string type = "";
			uf::asset::Payload payload = {};
		};

	#if UF_COMPONENT_POINTERED_USERDATA
		typedef pod::PointeredUserdata userdata_t;
	#else
		typedef pod::Userdata* userdata_t;
	#endif

		extern UF_API bool assertionLoad;
		extern UF_API uf::stl::unordered_map<uf::stl::string, uf::asset::userdata_t> map;
		extern UF_API Job::container_t jobs;
		extern UF_API uf::Serializer metadata;
	
	//	extern UF_API uf::Serializer map;

		uf::asset::Payload UF_API resolveToPayload( const uf::stl::string&, const uf::stl::string& = "" );
		bool UF_API isExpected( const uf::asset::Payload&, uf::asset::Type expected );

		// URL or file path
		void UF_API processQueue();

		void UF_API cache( const uf::stl::string&, const uf::asset::Payload& );
		void UF_API load( const uf::stl::string&, const uf::asset::Payload& );

		uf::stl::string UF_API cache( uf::asset::Payload& );
		uf::stl::string UF_API load( uf::asset::Payload& );

		bool has( const uf::stl::string& url );
		bool has( const uf::asset::Payload& payload );

		uf::asset::userdata_t& get( const uf::stl::string& url );
		uf::asset::userdata_t release( const uf::stl::string& url );
		void remove( const uf::stl::string& url );

		template<typename T>
		T& get( const uf::stl::string& url ) {
			if ( !uf::asset::has( url ) ) {
			#if UF_COMPONENT_POINTERED_USERDATA
				uf::asset::map[url] = uf::pointeredUserdata::create<T>();
			#else
				uf::asset::map[url] = uf::userdata::create<T>();
			#endif
			}

		#if UF_COMPONENT_POINTERED_USERDATA
			return uf::pointeredUserdata::get<T>( uf::asset::map[url] );
		#else
			return uf::userdata::get<T>( uf::asset::map[url] );
		#endif
		}
		template<typename T>
		T& get( uf::asset::Payload& payload ) {
			return payload.asComponent && payload.object ? uf::Entity::resolve( payload.object ).getComponent<T>() : uf::asset::get<T>( payload.filename );
		}

		template<typename T>
		T& add( const uf::stl::string& url ) {
		#if UF_COMPONENT_POINTERED_USERDATA
			uf::asset::map[url] = uf::pointeredUserdata::create<T>();
		#else
			uf::asset::map[url] = uf::userdata::create<T>();
		#endif
			return uf::asset::get<T>( url );
		}
		template<typename T>
		T& add( const uf::stl::string& url, const T& copy ) {
		#if UF_COMPONENT_POINTERED_USERDATA
			uf::asset::map[url] = uf::pointeredUserdata::create<T>( copy );
		#else
			uf::asset::map[url] = uf::userdata::create<T>( copy );
		#endif
			return uf::asset::get<T>( url );
		}

	}
}

namespace pod {
	namespace payloads {
		typedef uf::asset::Payload assetLoad;
	}
}