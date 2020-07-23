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

		template<typename T>
		void add( const std::string& name ) {
			if ( !names ) names = new std::unordered_map<std::type_index, std::string>;
			if ( !map ) map = new std::unordered_map<std::string, function_t>;

			auto& names = *uf::instantiator::names;
			auto& map = *uf::instantiator::map;

			names[std::type_index(typeid(T))] = name;
			map[name] = [](){
				return new T;
			};

			std::cout << "Registered instantiation for " << name << std::endl;
		}
		uf::Entity* UF_API instantiate( const std::string& );
		template<typename T>
		T& instantiate() {
			auto& names = *uf::instantiator::names;
			return *((T*) uf::instantiator::instantiate( names[std::type_index(typeid(T))] ));
		}
	};
}