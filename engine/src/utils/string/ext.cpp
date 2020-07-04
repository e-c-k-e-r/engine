#include <uf/utils/string/ext.h>
#include <sys/stat.h>

std::string UF_API uf::string::filename( const std::string& str ) {
	return str.substr( str.find_last_of('/') + 1 );
}
std::string UF_API uf::string::extension( const std::string& str ) {
	return str.substr( str.find_last_of('.') + 1 );
}
std::string UF_API uf::string::directory( const std::string& str ) {
	return str.substr( 0, str.find_last_of('/') ) + "/";
}
std::vector<std::string> UF_API uf::string::split( const std::string& haystack, const std::string& needle ) {
	std::vector<std::string> cont;
	std::size_t current, previous = 0;
    current = haystack.find_first_of(needle);
    while (current != std::string::npos) {
        cont.push_back(haystack.substr(previous, current - previous));
        previous = current + 1;
        current = haystack.find_first_of(needle, previous);
    }
    cont.push_back(haystack.substr(previous, current - previous));
    return cont;
}
std::string UF_API uf::string::replace( const std::string& string, const std::string& search, const std::string& replace ) {
	std::string result = string;
	size_t start_pos = string.find(search);
    if( start_pos == std::string::npos ) return result;
    result.replace(start_pos, search.length(), replace);
    return result;
}
bool UF_API uf::string::exists( const std::string& str ) {
	static struct stat buffer;
	return stat(str.c_str(), &buffer) == 0;
}