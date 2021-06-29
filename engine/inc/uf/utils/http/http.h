#pragma once

#include <uf/config.h>
#include <uf/utils/memory/string.h>

namespace uf {
	struct UF_API Http {
		uf::stl::string header;
		uf::stl::string response;
		char* effective;
		long code;
		double elapsed;
	};
	namespace http {
		uf::Http UF_API get( const uf::stl::string& );
	}
}