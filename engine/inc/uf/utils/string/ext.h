#pragma once

#include <uf/config.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>
#include <sstream>

#include "io.h"

namespace uf {
	namespace string {

		uf::stl::string UF_API replace( const uf::stl::string&, const uf::stl::string&, const uf::stl::string& );
		uf::stl::string UF_API lowercase( const uf::stl::string& );
		uf::stl::string UF_API uppercase( const uf::stl::string& );
		uf::stl::vector<uf::stl::string> UF_API split( const uf::stl::string&, const uf::stl::string& );
		uf::stl::string UF_API si( double value, const uf::stl::string& unit, size_t precision = 3 );
		bool UF_API contains( const uf::stl::string&, const uf::stl::string& );

		template<typename T>
		uf::stl::string /*UF_API*/ join( const T&, const uf::stl::string& = "\n", bool = false );
		template<typename T>
		uf::stl::string /*UF_API*/ toString( const T& );
	}
}

#include "ext.inl"