#pragma once

#include <uf/config.h>
#include <string>
#include <vector>
#include <sstream>

#include "io.h"

namespace uf {
	namespace string {

		std::string UF_API replace( const std::string&, const std::string&, const std::string& );
		std::string UF_API lowercase( const std::string& );
		std::string UF_API uppercase( const std::string& );
		std::vector<std::string> UF_API split( const std::string&, const std::string& );
		std::string UF_API si( double value, const std::string& unit, size_t precision = 3 );
		

		template<typename T>
		std::string /*UF_API*/ join( const T&, const std::string& = "\n", bool = false );
		template<typename T>
		std::string /*UF_API*/ toString( const T& );
	}
}

#include "ext.inl"