#pragma once

#include <uf/config.h>
#include <string>

namespace uf {
	struct UF_API Http {
		std::string header;
		std::string response;
		char* effective;
		long code;
		double elapsed;
	};
	namespace http {
		uf::Http UF_API get( const std::string& );
	}
}