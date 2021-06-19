#pragma once

#include <uf/utils/http/http.h>
#include <uf/engine/entity/entity.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/hook/hook.h>
#include <unordered_map>
#include <functional>

namespace uf {
	class UF_API Asset : public uf::Component {
	protected:
		static uf::Asset masterAssetLoader;
	public:
		// URL or file path
		void processQueue();

		void cache( const std::string&, const std::string&, const std::string&, const std::string& );
		void load( const std::string&, const std::string&, const std::string&, const std::string& );

		std::string cache( const std::string&, const std::string& = "", const std::string& = "" );
		std::string load( const std::string&, const std::string& = "", const std::string& = "" );

		std::string getOriginal( const std::string& );

		template<typename T>
		std::vector<T>& getContainer() {
			return this->getComponent<std::vector<T>>();
		}

		template<typename T>
		bool has( std::size_t i = 0 ) {
			auto& container = this->getContainer<T>();
			return container.size() > i;
		}
		template<typename T>
		bool has( const std::string& url ) {
			auto& container = this->getContainer<T>();
			if ( container.empty() ) return false;
			std::string extension = uf::io::extension( url );
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
		T& get( const std::string& url ) {
			std::string extension = uf::io::extension( url );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			size_t index = map[extension][url]["index"].as<size_t>(0);
			return this->get<T>(index);
		}

		template<typename T>
		T& add( const std::string& url, const T& copy ) {
			std::string extension = uf::io::extension( url );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			auto& container = this->getContainer<T>();
			if ( !ext::json::isNull( map[extension][url]["index"] ) ) return this->get<T>(url);
			
			container.push_back( copy );
			return container.back();
		}
		template<typename T>
		T& add( const std::string& url, T&& move ) {
			std::string extension = uf::io::extension( url );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			auto& container = this->getContainer<T>();

			if ( !ext::json::isNull( map[extension][url]["index"] ) ) return this->get<T>(url);
		
			container.push_back( move );
			return container.back();
		}

		template<typename T>
		void remove( const std::string& url ) {
			if ( !this->has<T>( url ) ) return;
			auto& container = this->getContainer<T>();

			std::string extension = uf::io::extension( url );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			std::size_t index = map[extension][url]["index"].as<size_t>();
		//	container.erase( container.begin() + index );
		//	map[extension][url] = ext::json::null();
		//	map[extension][url]["erased"] = true;
			
			std::string key = "";
			ext::json::forEach( map[extension], [&]( const std::string& k, ext::json::Value& v ) {
				std::size_t i = v["index"].as<size_t>();
				if ( index == i && key != url ) key = k;
			});
			if ( key != "" ) map[extension][key] = ext::json::null();
			map[extension][url] = ext::json::null();
			return;
		}
	};
}