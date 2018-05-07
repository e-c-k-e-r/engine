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
bool UF_API uf::string::exists( const std::string& str ) {
	static struct stat buffer;
	return stat(str.c_str(), &buffer) == 0;
}