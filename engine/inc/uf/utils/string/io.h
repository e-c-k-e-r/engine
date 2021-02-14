#pragma once

#include <uf/config.h>
#include <string>
#include <vector>

namespace uf {
	namespace io {
		extern UF_API const std::string root;

		std::string UF_API absolute( const std::string& );
		std::string UF_API filename( const std::string& );
		std::string UF_API extension( const std::string& );
		std::string UF_API directory( const std::string& );
		std::string UF_API sanitize( const std::string&, const std::string& = "" );
		std::string UF_API readAsString( const std::string&, const std::string& = "" );
		std::vector<uint8_t> UF_API readAsBuffer( const std::string&, const std::string& = "" );
		std::string UF_API hash( const std::string& );
		bool UF_API exists( const std::string& );
		size_t UF_API mtime( const std::string& );
		std::string UF_API resolveURI( const std::string&, const std::string& = "" );
	}
}