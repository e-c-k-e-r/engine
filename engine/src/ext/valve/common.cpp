#include <uf/ext/valve/common.h>

bool impl::parseKeyValue( const uf::stl::string& line, uf::stl::string& key, uf::stl::string& value ) {
	size_t q1 = line.find('"'); if ( q1 == uf::stl::string::npos ) return false;
	size_t q2 = line.find('"', q1 + 1); if ( q2 == uf::stl::string::npos ) return false;
	size_t q3 = line.find('"', q2 + 1); if ( q3 == uf::stl::string::npos ) return false;
	size_t q4 = line.find('"', q3 + 1); if ( q4 == uf::stl::string::npos ) return false;

	key = line.substr( q1 + 1, q2 - q1 - 1 );
	value = line.substr( q3 + 1, q4 - q3 - 1 );
	return true;
}