#pragma once

#include <uf/config.h>
#include <string>

namespace uf {
	namespace string {

		std::string UF_API filename( const std::string& );
		std::string UF_API extension( const std::string& );
		std::string UF_API directory( const std::string& );
		bool UF_API exists( const std::string& );
	}
}