#pragma once

#include <uf/utils/http/http.h>
#include <uf/engine/entity/entity.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/memory/unordered_map.h>
#include <functional>

namespace uf {
	class UF_API Asset : public uf::Component {
	protected:
		static uf::Asset masterAssetLoader;
	public:
		enum Type {
			UNKNOWN,
			IMAGE,
			AUDIO,
			LUA,
			JSON,
			GRAPH,
		};

		struct Payload {
			uf::Asset::Type type = {};
			uf::stl::string filename = "";
			uf::stl::string mime = "";
			uf::stl::string hash = "";

			bool initialize = true;
			bool monoThreaded = false;
			size_t uid = 0;
		};

		static uf::Asset::Payload resolveToPayload( const uf::stl::string&, const uf::stl::string& = "" );
		static bool isExpected( const uf::Asset::Payload&, uf::Asset::Type expected );

		static bool assertionLoad;

		// URL or file path
		void processQueue();

		void cache( const uf::stl::string&, const uf::Asset::Payload& );
		void load( const uf::stl::string&, const uf::Asset::Payload& );

		uf::stl::string cache( const uf::Asset::Payload& );
		uf::stl::string load( const uf::Asset::Payload& );

		uf::stl::string getOriginal( const uf::stl::string& );

		template<typename T>
		uf::stl::vector<T>& getContainer() {
			return this->getComponent<uf::stl::vector<T>>();
		}

		template<typename T>
		bool has( std::size_t i = 0 ) {
			auto& container = this->getContainer<T>();
			return container.size() > i;
		}
		template<typename T>
		bool has( const uf::stl::string& url ) {
			auto& container = this->getContainer<T>();
			if ( container.empty() ) return false;
			uf::stl::string extension = uf::io::extension( url, -1 );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			if ( ext::json::isNull( map[extension] ) ) return false;
			if ( ext::json::isNull( map[extension][url] ) ) return false;
			if ( ext::json::isNull( map[extension][url]["index"] ) ) return false;
			return true;
		}
		template<typename T>
		T& get( std::size_t i = 0 ) {
			auto& container = this->getContainer<T>();
			return container.at(i);
		}
		template<typename T>
		T& get( const uf::stl::string& url ) {
			uf::stl::string extension = uf::io::extension( url, -1 );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			size_t index = map[extension][url]["index"].as<size_t>(0);
			return this->get<T>(index);
		}

		template<typename T>
		T& add( const uf::stl::string& url, const T& copy ) {
			uf::stl::string extension = uf::io::extension( url, -1 );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			auto& container = this->getContainer<T>();
			if ( !ext::json::isNull( map[extension][url]["index"] ) ) return this->get<T>(url);
			
			container.push_back( copy );
			return container.back();
		}
		template<typename T>
		T& add( const uf::stl::string& url, T&& move ) {
			uf::stl::string extension = uf::io::extension( url, -1 );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			auto& container = this->getContainer<T>();

			if ( !ext::json::isNull( map[extension][url]["index"] ) ) return this->get<T>(url);
		
			container.push_back( move );
			return container.back();
		}

		template<typename T>
		void remove( const uf::stl::string& url ) {
			if ( !this->has<T>( url ) ) return;
			auto& container = this->getContainer<T>();

			uf::stl::string extension = uf::io::extension( url, -1 );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			std::size_t index = map[extension][url]["index"].as<size_t>();
		//	container.erase( container.begin() + index );
		//	map[extension][url] = ext::json::null();
		//	map[extension][url]["erased"] = true;
			
			uf::stl::string key = "";
			ext::json::forEach( map[extension], [&]( const uf::stl::string& k, ext::json::Value& v ) {
				std::size_t i = v["index"].as<size_t>();
				if ( index == i && key != url ) key = k;
			});
			if ( key != "" ) map[extension][key] = ext::json::null();
			map[extension][url] = ext::json::null();
			return;
		}
	};
}

namespace pod {
	namespace payloads {
		typedef uf::Asset::Payload assetLoad;
	}
}