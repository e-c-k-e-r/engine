#pragma once

#include <uf/config.h>
#include <string>
#include <vector>

namespace uf {
	namespace string {

		std::string UF_API filename( const std::string& );
		std::string UF_API extension( const std::string& );
		std::string UF_API directory( const std::string& );
		std::string UF_API sanitize( const std::string&, const std::string& = "" );
		std::string UF_API replace( const std::string&, const std::string&, const std::string& );
		std::string UF_API lowercase( const std::string& );
		std::string UF_API uppercase( const std::string& );
		std::vector<std::string> UF_API split( const std::string&, const std::string& );
		bool UF_API exists( const std::string& );
		size_t UF_API mtime( const std::string& );
	}
}