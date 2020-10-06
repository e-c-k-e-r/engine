#include <uf/utils/string/ext.h>
#include <algorithm>

std::string UF_API uf::string::lowercase( const std::string& str ) {
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower;
}
std::string UF_API uf::string::uppercase( const std::string& str ) {
	std::string upper = str;
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
	return upper;
}
std::vector<std::string> UF_API uf::string::split( const std::string& haystack, const std::string& needle ) {
	std::vector<std::string> cont;
	size_t last = 0, next = 0;
	while ((next = haystack.find(needle, last)) != std::string::npos) {
		cont.push_back(haystack.substr(last, next-last));
		last = next + 1;
	}
	cont.push_back( haystack.substr(last) );
	return cont;
}
std::string UF_API uf::string::replace( const std::string& string, const std::string& search, const std::string& replace ) {
	std::string result = string;
	size_t start_pos = string.find(search);
	if( start_pos == std::string::npos ) return result;
	result.replace(start_pos, search.length(), replace);
	return result;
}