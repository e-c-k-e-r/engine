#pragma once

#include <uf/config.h>
#include <uf/utils/memory/vector.h>

#if UF_USE_TOML
namespace ext {
	namespace toml {
		
		uf::stl::string UF_API toJson( const uf::stl::string& );
		uf::stl::vector<uint8_t> UF_API toJson( const uf::stl::vector<uint8_t>& );
		
		uf::stl::string UF_API fromJson( const uf::stl::string& );
		uf::stl::vector<uint8_t> UF_API fromJson( const uf::stl::vector<uint8_t>& );
	}
}
#endif