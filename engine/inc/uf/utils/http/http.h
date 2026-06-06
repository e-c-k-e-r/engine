#pragma once

#include <uf/config.h>
#include <uf/utils/memory/string.h>

namespace uf {
	struct UF_API Http {
		uf::stl::string header;
		uf::stl::string response;
		size_t contentLength;
		size_t mtime;

		char* effective;
		long code;
		double elapsed;
	};
	namespace http {
		uf::Http UF_API get( const uf::stl::string& );
		uf::Http UF_API head( const uf::stl::string& );
		uf::Http UF_API post( const uf::stl::string&, const void* data, size_t len );
	}
}