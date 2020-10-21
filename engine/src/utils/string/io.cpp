#include <uf/utils/string/io.h>
#include <uf/utils/string/ext.h>

#include <sys/stat.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <uf/utils/io/iostream.h>

#ifdef WINDOWS
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

std::string UF_API uf::io::absolute( const std::string& path ) {
	char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	return std::string(buff) + path;
}
std::string UF_API uf::io::filename( const std::string& str ) {
	return str.substr( str.find_last_of('/') + 1 );
}
std::string UF_API uf::io::extension( const std::string& str ) {
	return str.substr( str.find_last_of('.') + 1 );
}
std::string UF_API uf::io::directory( const std::string& str ) {
	return str.substr( 0, str.find_last_of('/') ) + "/";
}
std::string UF_API uf::io::sanitize( const std::string& str, const std::string& root ) {
	std::string path = str;
	// append root to path
	if ( path.find(root) == std::string::npos ) {
		path = root + "/" + path;
	}
	// flatten all "/./"
	{
		std::string tmp;
		while ( path != (tmp = uf::string::replace(path, "/./", "/")) ) {
			path = tmp;
		}
	}
	// flatten all "//"
	{
		std::string tmp;
		while ( path != (tmp = uf::string::replace(path, "//", "/")) ) {
			path = tmp;
		}
	}
	return path;
}
std::string UF_API uf::io::readAsString( const std::string& filename ) {
	std::string buffer;
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
	if ( !is.is_open() ) {
		uf::iostream << "Error: Could not open file \"" << filename << "\"" << "\n";
		return buffer;
	}
	is.seekg(0, std::ios::end); buffer.reserve(is.tellg()); is.seekg(0, std::ios::beg);
	buffer.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	return buffer;
}
std::vector<uint8_t> UF_API uf::io::readAsBuffer( const std::string& filename ) {
	std::vector<uint8_t> buffer;
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
	if ( !is.is_open() ) {
		uf::iostream << "Error: Could not open file \"" << filename << "\"" << "\n";
		return buffer;
	}
	is.seekg(0, std::ios::end); buffer.reserve(is.tellg()); is.seekg(0, std::ios::beg);
	buffer.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	return buffer;
}
bool UF_API uf::io::exists( const std::string& filename ) {
	static struct stat buffer;
	return stat(filename.c_str(), &buffer) == 0;
}
size_t UF_API uf::io::mtime( const std::string& filename ) {
	static struct stat buffer;
	if ( stat(filename.c_str(), &buffer) != 0 ) return -1;
	return buffer.st_mtime;
}