#include <uf/ext/ttlg/common.h>

uf::stl::string impl::sanitizeString( const char* raw, size_t maxLength ) {
	return uf::stl::string( raw );
/*
	if (!raw || maxLength == 0) return "";

	size_t len = 0;
	while ( len < maxLength && raw[len] != '\0' ) ++len;

	uf::stl::string clean;
	clean.reserve(len);
	for ( size_t i = 0; i < len; ++i ) {
		unsigned char c = (unsigned char)(raw[i]);
		if ( c >= 32 && c <= 126 ) {
			clean += c;
		} else {
			if (c == '\t') clean += "\\t";
			else if (c == '\n') clean += "\\n";
			else if (c == '\r') clean += "\\r";
		}
	}
	return clean;
*/
}