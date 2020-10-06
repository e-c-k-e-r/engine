#pragma once

#include <uf/config.h>
#include <string>
#include <vector>

#include "io.h"

namespace uf {
	namespace string {
		std::string UF_API replace( const std::string&, const std::string&, const std::string& );
		std::string UF_API lowercase( const std::string& );
		std::string UF_API uppercase( const std::string& );
		std::vector<std::string> UF_API split( const std::string&, const std::string& );
	}
}