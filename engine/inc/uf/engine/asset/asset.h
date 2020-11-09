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
		std::string cache( const std::string& );
		std::string load( const std::string& );
		std::string getOriginal( const std::string& );
		void cache( const std::string&, const std::string& );
		void load( const std::string&, const std::string& );

		template<typename T>
		std::vector<T>& getContainer() {
			return this->getComponent<std::vector<T>>();
		}

		template<typename T>
		T& has( std::size_t i = 0 ) {
			auto& container = this->getContainer<T>();
			return container.size() > i;
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
		
			std::size_t index = map[extension][url].as<size_t>();
		//	std::size_t index = map[extension][url];
		
			return this->get<T>(index);
		}

		template<typename T>
		T& add( const std::string& url, const T& copy ) {
			std::string extension = uf::io::extension( url );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			auto& container = this->getContainer<T>();
			if ( !ext::json::isNull( map[extension][url] ) ) return this->get<T>(url);
			
		//	sol::optional<std::string> value = map[extension][url];
		//	if ( !value ) return this->get<T>(url);

			container.push_back( copy );
			return container.back();
		}
		template<typename T>
		T& add( const std::string& url, T&& move ) {
			std::string extension = uf::io::extension( url );
			uf::Serializer& map = this->getComponent<uf::Serializer>();
			auto& container = this->getContainer<T>();

			if ( !ext::json::isNull( map[extension][url] ) ) return this->get<T>(url);
		
		//	sol::optional<std::string> value = map[extension][url];
		//	if ( !value ) return this->get<T>(url);
		
			container.push_back( move );
			return container.back();
		}
	};
}