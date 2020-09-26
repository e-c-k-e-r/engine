#pragma once

#include <uf/config.h>
#include <uf/engine/entity/entity.h>
#include <uf/utils/singletons/pre_main.h>
#include <unordered_map>
#include <typeindex>
#include <functional>

#define NAMESPACE_CONCAT
#define UF_OBJECT_REGISTER_CPP( OBJ ) \
namespace {\
	static uf::StaticInitialization REGISTER_UF_ ## OBJ( []{\
		uf::instantiator::add<uf::OBJ>( #OBJ );\
	});\
}
#define EXT_OBJECT_REGISTER_CPP( OBJ ) \
namespace {\
	static uf::StaticInitialization REGISTER_EXT_ ## OBJ( []{\
		uf::instantiator::add<ext::OBJ>( #OBJ );\
	});\
}

namespace uf {
	namespace instantiator {
		typedef std::function<uf::Entity*()> function_t;
		extern UF_API std::unordered_map<std::type_index, std::string>* names;
		extern UF_API std::unordered_map<std::string, function_t>* map;

		uf::Entity* UF_API alloc( size_t );
		void UF_API free( uf::Entity* );
		uf::Entity* UF_API instantiate( const std::string& );

		template<typename T> void add( const std::string& name );

		template<typename T> T* alloc();
		template<typename T> T& instantiate( size_t size );
		template<typename T> T& instantiate();
		template<typename T> T* _instantiate();
	};
}

#include "instantiator.inl"