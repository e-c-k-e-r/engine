#pragma once

#include <uf/config.h>
#include <uf/engine/entity/entity.h>
#include <uf/utils/singletons/pre_main.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/memory/unordered_map.h>
#include <typeindex>
#include <functional>

#define UF_INSTANTIATOR_ANNOUNCE 0

namespace pod {
	struct UF_API Instantiator {
		typedef std::function<uf::Entity*()> function_t;
		typedef uf::stl::vector<uf::stl::string> behaviors_t;
		function_t function;
		behaviors_t behaviors;
	};

	template<typename C>
	struct UF_API NamedTypes {
		typedef std::type_index type_t;
		typedef uf::stl::unordered_map<type_t, uf::stl::string> container_t;
		typedef uf::stl::unordered_map<uf::stl::string, C> map_t;
		container_t names;
		map_t map;

		type_t getType( const uf::stl::string& name );
		template<typename T> type_t getType();
		template<typename T> uf::stl::string getName();

		bool has( const uf::stl::string& name, bool = true );
		template<typename T> bool has( bool = true );

		template<typename T> void add( const uf::stl::string& name, const C& c );

		C& get( const uf::stl::string& name );
		template<typename T> C& get();

		C& operator[]( const uf::stl::string& name );
	};
}

namespace uf {
	namespace instantiator {
		extern UF_API pod::NamedTypes<pod::Instantiator>* objects;
	//	extern UF_API pod::NamedTypes<pod::Behavior>* behaviors;
		extern UF_API uf::stl::unordered_map<uf::stl::string, pod::Behavior>* behaviors;

		uf::Entity* UF_API alloc( size_t );
		template<typename T> T* alloc();
		void UF_API free( uf::Entity* );
		bool UF_API valid( uf::Entity* );

		uf::Entity* UF_API reuse( size_t );
		size_t UF_API collect( uint8_t = 0 );

		template<typename T> void registerObject( const uf::stl::string& name );
	//	template<typename T> void registerBehavior( const uf::stl::string& name );
		template<typename T> void registerBinding( const uf::stl::string& name );
		void UF_API registerBehavior( const uf::stl::string& name, const pod::Behavior& );
		void UF_API registerBinding( const uf::stl::string& object, const uf::stl::string& behavior );

		uf::Entity& UF_API instantiate( const uf::stl::string& );
		template<typename T> T& instantiate();
		template<typename T> T* _instantiate();

		void UF_API bind( const uf::stl::string&, uf::Entity& );
		template<typename T> void bind( uf::Entity& );

		void UF_API unbind( const uf::stl::string&, uf::Entity& );
		template<typename T> void unbind( uf::Entity& );
	};
}

#include "instantiator.inl"
#include "macros.inl"