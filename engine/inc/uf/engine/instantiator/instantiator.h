#pragma once

#include <uf/config.h>
#include <uf/engine/entity/entity.h>
#include <uf/utils/singletons/pre_main.h>
#include <unordered_map>
#include <typeindex>
#include <functional>

#define UF_INSTANTIATOR_ANNOUNCE 0

namespace pod {
	struct UF_API Instantiator {
		typedef std::function<uf::Entity*()> function_t;
		typedef std::vector<std::string> behaviors_t;
		function_t function;
		behaviors_t behaviors;
	};

	template<typename C>
	struct UF_API NamedTypes {
		typedef std::type_index type_t;
		typedef std::unordered_map<type_t, std::string> container_t;
		typedef std::unordered_map<std::string, C> map_t;
		container_t names;
		map_t map;

		type_t getType( const std::string& name );
		template<typename T> type_t getType();
		template<typename T> std::string getName();

		bool has( const std::string& name, bool = true );
		template<typename T> bool has( bool = true );

		template<typename T> void add( const std::string& name, const C& c );

		C& get( const std::string& name );
		template<typename T> C& get();
	};
}

namespace uf {
	namespace instantiator {
		extern UF_API pod::NamedTypes<pod::Instantiator>* objects;
		extern UF_API pod::NamedTypes<pod::Behavior>* behaviors;

		uf::Entity* UF_API alloc( size_t );
		template<typename T> T* alloc();
		void UF_API free( uf::Entity* );

		template<typename T> void registerObject( const std::string& name );
		template<typename T> void registerBehavior( const std::string& name );
		template<typename T> void registerBinding( const std::string& name );
		void UF_API registerBinding( const std::string& object, const std::string& behavior );

		uf::Entity& UF_API instantiate( const std::string& );
		template<typename T> T& instantiate();
		template<typename T> T* _instantiate();

		void UF_API bind( const std::string&, uf::Entity& );
		template<typename T> void bind( uf::Entity& );
		void UF_API unbind( const std::string&, uf::Entity& );
		template<typename T> void unbind( uf::Entity& );
	};
}

#include "instantiator.inl"
#include "macros.inl"